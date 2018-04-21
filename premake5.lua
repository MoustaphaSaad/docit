-- [[ function returning the sdk version of windows 10 --]]
function win10_sdk_version()
	cmd_file = io.popen("reg query \"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots\" | C:\\Windows\\System32\\find.exe \"10.0\"", 'r')
	output = cmd_file:read("*all")
	cmd_file:close()
	out_root, out_leaf, out_ext = string.match(output, "(.-)([^\\]-([^%.]+))$")
	sdk_version = out_leaf:gsub("%s+", "")
	return sdk_version
end

bin_path 		= path.getabsolute("bin")
build_path 		= path.getabsolute("build")

workspace "docit"
	configurations {"debug", "release"}
	platforms {"x86", "x64"}
	location "build"
	startproject "docit"

	project "docit"
		language "C++"
		kind "ConsoleApp"
		targetdir (bin_path .. "/%{cfg.platform}/%{cfg.buildcfg}/")
		location  (build_path .. "/docit/")

		files
		{
			"include/**.h",
			"src/**.cpp"
		}

		includedirs {
			"include/"
		}

		links {"libclang"}

		if os.istarget("linux") then

			buildoptions {"-std=c++14", "-Wall", "-fno-rtti", "-fno-exceptions"}
			linkoptions {"-pthread"}

			filter "configurations:debug"
				linkoptions {"-rdynamic"}

		elseif os.istarget("windows") then
			
			if os.getversion().majorversion == 10.0 then
				systemversion(win10_sdk_version())
			end

			buildoptions {"/utf-8"}

			includedirs
			{
				os.getenv("LLVM_DIR") .. "/include/"
			}

			libdirs
			{
				os.getenv("LLVM_DIR") .. "/lib/"
			}
		end

		filter "configurations:debug"
			defines {"DEBUG"}
			symbols "On"

		filter "configurations:release"
			defines {"NDEBUG"}
			optimize "On"

		filter "platforms:x86"
			architecture "x32"

		filter "platforms:x64"
			architecture "x64"
