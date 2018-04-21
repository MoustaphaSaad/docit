#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cctype>
#include <algorithm>
#include <clang-c/Index.h>
using namespace std;

struct Docit_State;

//debug funcitons
void
show_clang_version()
{
	CXString version = clang_getClangVersion();
	printf("%s\n", clang_getCString(version));
	clang_disposeString(version);
}

void
cursor_show_spelling(CXCursor* node)
{
	CXString spelling = clang_getCursorSpelling(*node);
	printf("Spelling:%s\n", clang_getCString(spelling));
	clang_disposeString(spelling);
}

void
token_show_spelling(CXTranslationUnit* translation_unit, CXToken* token)
{
	CXString spelling = clang_getTokenSpelling(*translation_unit, *token);
	printf("Spelling:%s\n", clang_getCString(spelling));
	clang_disposeString(spelling);
}

void
cursor_print_extent(const std::vector<std::string>& file_lines, CXCursor* node)
{
	CXCursorKind cursor_kind = clang_getCursorKind(*node);
	if (cursor_kind == CXCursorKind::CXCursor_ClassTemplate ||
		cursor_kind == CXCursorKind::CXCursor_ClassTemplatePartialSpecialization ||
		cursor_kind == CXCursorKind::CXCursor_ClassDecl ||
		cursor_kind == CXCursorKind::CXCursor_StructDecl)
	{
		CXSourceRange range = clang_getCursorExtent(*node);
		CXSourceLocation begin = clang_getRangeStart(range);
		unsigned int line = 0;

		clang_getFileLocation(begin, nullptr, &line, nullptr, nullptr);

		printf("%s\n", file_lines[line].c_str());
	}
	else
	{
		CXSourceRange range = clang_getCursorExtent(*node);
		CXSourceLocation begin = clang_getRangeStart(range);
		CXSourceLocation end = clang_getRangeEnd(range);
		unsigned int begin_line = 0, end_line = 0;
		clang_getFileLocation(begin, nullptr, &begin_line, nullptr, nullptr);
		clang_getFileLocation(end, nullptr, &end_line, nullptr, nullptr);
		for (unsigned int i = begin_line - 1; i < end_line; ++i)
			printf("%s\n", file_lines[i].c_str());
	}
}


//libclang main functions
CXIndex
create_index()
{
	return clang_createIndex(1, 1);
}

CXTranslationUnit
create_translation_unit(const CXIndex& index, const char* filename, int argc, char** argv)
{
	CXTranslationUnit result = nullptr;
	auto debug = clang_parseTranslationUnit2(index,
			filename,
			argv,	//command line arguments
			argc,	//command line arguments count
			nullptr,	//unsaved files
			0,			//unsaved files count
			CXTranslationUnit_SingleFileParse|		//we process only a single file
			CXTranslationUnit_KeepGoing|			//don't stop on fatal errors
			CXTranslationUnit_SkipFunctionBodies|	//skip implementations
			CXTranslationUnit_Incomplete,			//turn off semantic analysis
			&result);
	return result;
}


//string processing
bool
str_contains(const char* str, const char* pattern)
{
	auto pattern_it = pattern;
	while (str && *str != 0)
	{
		if (*pattern_it == *str)
			++pattern_it;
		else
			pattern_it = pattern;

		if (*pattern_it == 0)
			return true;
		++str;
	}
	return false;
}

bool
str_starts(const char* str, const char* pattern)
{
	while (str && *str != 0)
	{
		if (*pattern == *str)
			++pattern;
		else
			return false;

		if (*pattern == 0)
			return true;

		++str;
	}
	return false;
}

bool
is_default_documentation_comment(const char* str)
{
	return str_starts(str, "/**");
}

bool
is_markdown_comment(const char* str)
{
	return str_contains(str, "[[markdown]]");
}

bool
is_private(const char* str)
{
	return str_starts(str, "_");
}


//Docit section
struct Docit_Block
{
	enum TYPE { NONE, TOKEN, CURSOR };

	TYPE type;
	union
	{
		CXToken token;
		CXCursor cursor;
	};
};

Docit_Block
block_token(const CXToken& token)
{
	Docit_Block block;
	block.type = Docit_Block::TOKEN;
	block.token = token;
	return block;
}

Docit_Block
block_cursor(const CXCursor& cursor)
{
	Docit_Block block;
	block.type = Docit_Block::CURSOR;
	block.cursor = cursor;
	return block;
}

struct Docit_State
{
	const char* filename;
	CXIndex index;
	CXTranslationUnit translation_unit;
	CXToken* tokens;
	unsigned int tokens_count;
	std::vector<Docit_Block> blocks;
	std::vector<std::string> file_lines;
};

void
docit_dispose(Docit_State* docit)
{
	docit->filename = nullptr;

	docit->blocks.clear();

	if (docit->tokens != nullptr)
	{
		clang_disposeTokens(docit->translation_unit, docit->tokens, docit->tokens_count);
		docit->tokens_count = 0;
		docit->tokens = nullptr;
	}

	if (docit->translation_unit != nullptr)
	{
		clang_disposeTranslationUnit(docit->translation_unit);
		docit->translation_unit = nullptr;
	}
	if (docit->index != nullptr)
	{
		clang_disposeIndex(docit->index);
		docit->index = nullptr;
	}
}

bool
docit_init(Docit_State* docit, int argc, char** argv)
{
	if(argc > 0)
	{
		docit->filename = *argv;
		--argc;
		++argv;
	}
	else
	{
		printf("you must specify a file to process.\n\tex: docit [filename]\n");
		return false;
	}

	docit->index = create_index();
	docit->translation_unit = create_translation_unit(docit->index, docit->filename, argc, argv);
	if(docit->translation_unit == nullptr)
	{
		printf("cannot create translation unit of file: %s\n", docit->filename);
		docit_dispose(docit);
		return false;
	}

	return true;
}

void
docit_read_filelines(Docit_State* docit)
{
	std::ifstream stream(docit->filename);
	std::string line;
	while (std::getline(stream, line))
		docit->file_lines.emplace_back(line);
}

bool
is_documentation_comment(Docit_State* docit, size_t token_index)
{
	if (clang_getTokenKind(docit->tokens[token_index]) != CXTokenKind::CXToken_Comment)
		return false;
	
	bool result = false;

	CXString spelling = clang_getTokenSpelling(docit->translation_unit, docit->tokens[token_index]);
	const char* str = clang_getCString(spelling);
	if (is_default_documentation_comment(str) ||
		is_markdown_comment(str))
		result = true;
	clang_disposeString(spelling);

	return result;
}

void
docit_extract_comments(Docit_State* docit)
{
	CXCursor cursor = clang_getTranslationUnitCursor(docit->translation_unit);
	CXSourceRange cursor_range = clang_getCursorExtent(cursor);

	clang_tokenize(docit->translation_unit, cursor_range, &docit->tokens, &docit->tokens_count);

	for (size_t i = 0; i < docit->tokens_count; ++i)
		if (is_documentation_comment(docit, i))
			docit->blocks.emplace_back(block_token(docit->tokens[i]));
}

CXChildVisitResult
visit_children(CXCursor node, CXCursor parent, CXClientData user_data)
{
	Docit_State* docit = (Docit_State*)user_data;

	if (clang_isDeclaration(clang_getCursorKind(node)) &&
		clang_getCursorKind(node) != CXCursorKind::CXCursor_Namespace &&
		clang_getCursorKind(node) != CXCursorKind::CXCursor_ParmDecl)
	{

		// CXString spelling = clang_getCursorSpelling(node);
		// if (!is_private(clang_getCString(spelling)))
			docit->blocks.emplace_back(block_cursor(node));
		// clang_disposeString(spelling);
	}

	clang_visitChildren(node, visit_children, user_data);
	return CXChildVisitResult::CXChildVisit_Continue;
}

void
docit_extract_declaration(Docit_State* docit)
{
	CXCursor cursor = clang_getTranslationUnitCursor(docit->translation_unit);
	clang_visitChildren(cursor, visit_children, docit);
}

std::vector<std::string> string_split(const std::string& str,
									  const std::string& delimiter)
{
	std::vector<std::string> strings;

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while ((pos = str.find(delimiter, prev)) != std::string::npos)
	{
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}

	// To get the last substring (or only, if delimiter is not found)
	strings.push_back(str.substr(prev));

	return strings;
}

bool string_replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if(start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

void string_replace_all(std::string& str, const std::string& from, const std::string& to) {
	if(from.empty())
		return;
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

std::string string_ltrim(const std::string &s) {
	std::string result = s;
	result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](int ch) {
		return !std::isspace(ch);
	}));
	return result;
}

std::string string_rtrim(const std::string &s) {
	std::string result = s;
	result.erase(std::find_if(result.rbegin(), result.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), result.end());
	return result;
}



void
print_markdown_comment(std::stringstream& out, const char* str)
{
	std::vector<std::string> lines = string_split(str, "\n");

	for(auto& line: lines)
	{
		//check if this is the line to signal the markdown comment then we ignore it
		if (is_markdown_comment(line.c_str()) ||
			str_contains(line.c_str(), "/**") ||
			str_contains(line.c_str(), "*/"))
			continue;

		size_t position = line.find("*");
		if (position != std::string::npos)
			out << line.substr(position + 1);
		else
			out << line;
	}
}

void
print_documentation_comment(std::stringstream& out, const char* str)
{
	std::string comment(str);
	string_replace_all(comment, "@brief", "- **brief:**");
	string_replace_all(comment, "@param[in]", "- **param[in]:**");
	string_replace_all(comment, "@param[out]", "- **param[out]:**");
	string_replace_all(comment, "@param", "- **param:**");
	string_replace_all(comment, "@tparam", "- **tparam:**");
	string_replace_all(comment, "@return", "- **return:**");

	std::vector<std::string> lines = string_split(comment, "\n");
	for(auto& line: lines)
	{
		if (str_contains(line.c_str(), "/**") ||
			str_contains(line.c_str(), "*/"))
			continue;

		size_t position = line.find("*");
		if (position != std::string::npos)
			out << line.substr(position + 1);
		else
			out << line;
	}
}

void
export_comment(std::stringstream& out, Docit_State* docit, Docit_Block* block, bool* is_markdown)
{
	CXString spelling = clang_getTokenSpelling(docit->translation_unit, block->token);
	const char* str = clang_getCString(spelling);
	bool is_markdown_flag = is_markdown_comment(str);
	if(is_markdown)
		*is_markdown = is_markdown_flag;
	if(is_markdown_flag)
		print_markdown_comment(out, str);
	else
		print_documentation_comment(out, str);
	clang_disposeString(spelling);
}

void
export_code(std::stringstream& out, const std::vector<std::string>& file_lines, CXCursor* node)
{
	CXCursorKind cursor_kind = clang_getCursorKind(*node);
	if (cursor_kind == CXCursorKind::CXCursor_ClassTemplate ||
		cursor_kind == CXCursorKind::CXCursor_ClassTemplatePartialSpecialization)
	{
		CXSourceRange range = clang_getCursorExtent(*node);
		CXSourceLocation begin = clang_getRangeStart(range);
		unsigned int line = 0;

		clang_getFileLocation(begin, nullptr, &line, nullptr, nullptr);

		out << string_ltrim(file_lines[line]);
	}
	else if (cursor_kind == CXCursorKind::CXCursor_ClassDecl ||
		cursor_kind == CXCursorKind::CXCursor_StructDecl)
	{
		CXSourceRange range = clang_getCursorExtent(*node);
		CXSourceLocation begin = clang_getRangeStart(range);
		unsigned int line = 0;

		clang_getFileLocation(begin, nullptr, &line, nullptr, nullptr);

		if(line > 0)
			out << string_ltrim(file_lines[line - 1]);
	}
	else
	{
		CXSourceRange range = clang_getCursorExtent(*node);
		CXSourceLocation begin = clang_getRangeStart(range);
		CXSourceLocation end = clang_getRangeEnd(range);
		unsigned int begin_line = 0, end_line = 0;
		clang_getFileLocation(begin, nullptr, &begin_line, nullptr, nullptr);
		clang_getFileLocation(end, nullptr, &end_line, nullptr, nullptr);
		bool first_indentation = true;
		size_t first_indentation_count = 0;
		for (unsigned int i = begin_line - 1; i < end_line; ++i)
		{
			if (first_indentation)
			{
				first_indentation = false;
				for (first_indentation_count = 0; first_indentation_count < file_lines[i].size(); ++first_indentation_count)
					if (!std::isspace(file_lines[i][first_indentation_count]))
						break;
				out << file_lines[i].substr(first_indentation_count) << std::endl;
			}
			else
			{
				out << file_lines[i].substr(first_indentation_count) << std::endl;
			}
		}
	}
}

std::string
get_title_text(CXCursor* node)
{
	std::string result = "#";
	auto node_it = clang_getCursorSemanticParent(*node);
	while(!clang_equalCursors(node_it, clang_getNullCursor()))
	{
		auto kind = clang_getCursorKind(node_it);
		if (kind == CXCursorKind::CXCursor_StructDecl ||
			kind == CXCursorKind::CXCursor_ClassDecl ||
			kind == CXCursorKind::CXCursor_ClassTemplate ||
			kind == CXCursorKind::CXCursor_ClassTemplatePartialSpecialization)
			result += "#";

		node_it = clang_getCursorSemanticParent(node_it);
	}

	result += " ";

	auto cursor_kind = clang_getCursorKind(*node);
	if (cursor_kind == CXCursorKind::CXCursor_FunctionDecl ||
		cursor_kind == CXCursorKind::CXCursor_FunctionTemplate ||
		cursor_kind == CXCursorKind::CXCursor_CXXMethod)
	{
		if (clang_CXXMethod_isStatic(*node))
			result += "Static Funciton";
		else
			result += "Funciton";
	}
	else if (cursor_kind == CXCursorKind::CXCursor_Constructor)
	{
		if (clang_CXXConstructor_isCopyConstructor(*node))
			result += "Copy ";
		else if (clang_CXXConstructor_isMoveConstructor(*node))
			result += "Move ";
		else if (clang_CXXConstructor_isDefaultConstructor(*node))
			result += "Default ";
		else if (clang_CXXConstructor_isConvertingConstructor(*node))
			result += "Cast ";

		result += "Constructor";
	}
	else if (cursor_kind == CXCursorKind::CXCursor_StructDecl ||
			 cursor_kind == CXCursorKind::CXCursor_ClassDecl ||
			 cursor_kind == CXCursorKind::CXCursor_ClassTemplate ||
			 cursor_kind == CXCursorKind::CXCursor_ClassTemplatePartialSpecialization)
	{
		result += "Struct";
	}
	else if (cursor_kind == CXCursorKind::CXCursor_UnionDecl)
	{
		result += "Union";
	}
	else if (cursor_kind == CXCursorKind::CXCursor_FieldDecl)
	{
		result += "Variable";
	}
	else if (cursor_kind == CXCursorKind::CXCursor_TypeAliasDecl)
	{
		result += "Typedef";
	}
	else if (cursor_kind == CXCursorKind::CXCursor_ConversionFunction)
	{
		result += "Conversion Operator";
	}
	else
	{
		CXString kind_str = clang_getCursorKindSpelling(cursor_kind);
		result += std::string(clang_getCString(kind_str));
		clang_disposeString(kind_str);
	}

	CXString spelling = clang_getCursorSpelling(*node);
	result += " `";
	result += std::string(clang_getCString(spelling));
	result += "`";
	clang_disposeString(spelling);
	return result;
}

void
export_markdown(Docit_State* docit)
{
	for(size_t i = 0; i < docit->blocks.size(); ++i)
	{
		auto& block = docit->blocks[i];

		if(block.type == Docit_Block::TOKEN)
		{
			std::stringstream comment_stream;
			bool is_markdown = false;
			export_comment(comment_stream, docit, &block, &is_markdown);
			if(is_markdown)
			{
				printf("%s\n", comment_stream.str().c_str());
				continue;
			}

			//this is not a markdown comment so there should be a code under it
			bool found_code = false;
			std::stringstream code_stream;
			std::string title_text;
			++i;
			for(i; i < docit->blocks.size(); ++i)
			{
				block = docit->blocks[i];
				if(block.type == Docit_Block::CURSOR)
				{
					CXString spelling = clang_getCursorSpelling(block.cursor);
					if(is_private(clang_getCString(spelling)))
					{
						found_code = false;
						break;
						//clang_disposeString(spelling);
						//continue;
					}

					clang_disposeString(spelling);
					found_code = true;
					export_code(code_stream, docit->file_lines, &block.cursor);
					CXCursorKind cursor_kind = clang_getCursorKind(block.cursor);
					if (cursor_kind != CXCursorKind::CXCursor_TemplateTypeParameter)
					{
						title_text = get_title_text(&block.cursor);
						break;
					}
				}
			}

			if (found_code)
			{
				printf("%s\n", title_text.c_str());
				printf("```C++\n");
				auto code_str = string_rtrim(code_stream.str());

				
				if (code_str.back() != ';')
				{
					if (code_str.back() == '{')
						code_str.back() = ';';
					else if(code_str.back() != '}')
						code_str += ";";
				}

				printf("%s\n", code_str.c_str());
				printf("```\n");
				printf("%s\n", comment_stream.str().c_str());
			}
		}
	}
}


int main(int argc, char** argv)
{
	auto start = std::chrono::high_resolution_clock::now();
	//show_clang_version();
	//skip process name
	--argc;
	++argv;

	Docit_State docit;
	if (docit_init(&docit, argc, argv) == false)
		exit(-1);

	docit_read_filelines(&docit);
	//printf("Lines: %llu\n", docit.file_lines.size());
	docit_extract_comments(&docit);
	docit_extract_declaration(&docit);

	//sort the blocks by lines
	std::sort(docit.blocks.begin(), docit.blocks.end(), [&](const Docit_Block& a, const Docit_Block& b) {
		unsigned int a_line = 0, a_column = 0;
		unsigned int b_line = 0, b_column;

		switch (a.type)
		{
		case Docit_Block::CURSOR:
			{
				CXSourceRange range = clang_getCursorExtent(a.cursor);
				CXSourceLocation location = clang_getRangeStart(range);
				clang_getFileLocation(location, nullptr, &a_line, &a_column, nullptr);

				CXCursorKind cursor_kind = clang_getCursorKind(a.cursor);
				if (cursor_kind != CXCursorKind::CXCursor_ClassTemplate &&
					cursor_kind != CXCursorKind::CXCursor_ClassTemplatePartialSpecialization &&
					cursor_kind != CXCursorKind::CXCursor_ClassDecl &&
					cursor_kind != CXCursorKind::CXCursor_StructDecl)
				{
					--a_line;
				}
			}
			break;
		case Docit_Block::TOKEN:
			{
				CXSourceRange range = clang_getTokenExtent(docit.translation_unit, a.token);
				CXSourceLocation location = clang_getRangeStart(range);
				clang_getFileLocation(location, nullptr, &a_line, &a_column, nullptr);
			}
			break;
		default:
			break;
		}

		switch (b.type)
		{
			case Docit_Block::CURSOR:
			{
				CXSourceRange range = clang_getCursorExtent(b.cursor);
				CXSourceLocation location = clang_getRangeStart(range);
				clang_getFileLocation(location, nullptr, &b_line, &b_column, nullptr);

				CXCursorKind cursor_kind = clang_getCursorKind(b.cursor);
				if (cursor_kind != CXCursorKind::CXCursor_ClassTemplate &&
					cursor_kind != CXCursorKind::CXCursor_ClassTemplatePartialSpecialization &&
					cursor_kind != CXCursorKind::CXCursor_ClassDecl &&
					cursor_kind != CXCursorKind::CXCursor_StructDecl)
				{
					--b_line;
				}
			}
			break;
		case Docit_Block::TOKEN:
			{
				CXSourceRange range = clang_getTokenExtent(docit.translation_unit, b.token);
				CXSourceLocation location = clang_getRangeStart(range);
				clang_getFileLocation(location, nullptr, &b_line, &b_column, nullptr);
			}
			break;

		default:
			break;
		}

		if (a_line == b_line)
			return a_column < b_column;
		else
			return a_line < b_line;
	});

	//export the content into a markdown format
	export_markdown(&docit);
	docit_dispose(&docit);
	auto end = std::chrono::high_resolution_clock::now();
	double time_in_seconds = std::chrono::duration<double>(end - start).count();
	//printf("time: %g", time_in_seconds);
	return 0;
}