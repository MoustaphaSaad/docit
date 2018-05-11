namespace My_Namespace
{

	/**
	 * @brief      Given two variables it will return the minimum value
	 *
	 * @param[in]  a          The first value
	 * @param[in]  b          The second value
	 * @param[in]  compare_func  The compare function
	 *
	 * @tparam     T          Type of the values
	 * @tparam     TCompare   Type of the compare function
	 *
	 * @return     The minimum value of the two
	 */
	template<typename T, typename TCompare = default_less_than<T>>
	inline static const T&
	min(const T& a, const T& b, TCompare&& compare_func = TCompare())
	{
		if(compare_func(b, a))
			return b;
		return a;
	}

	/**
	 * @brief      This is my BaseA
	 */
	struct BaseA
	{
		/**
		 * @brief      This is my abstract function
		 *
		 * @param[in]  a     Some random number
		 * @param[in]  b     Some other random number
		 */
		virtual void
		my_abstract_function(int a,
							 int b) = 0;
	};

	namespace internal
	{
		struct My_Undocumented_Type
		{

		};
	}

	/**
	 * [[markdown]]
	 * ## Markdown Section
	 * This is a section that will be included in the output
	 * you can write anything markdown and it will be included but you have to put the
	 * 
	 * [[markdown]]  this text won't be included since it's on the same line as the tag
	 * 
	 * tag anywhere in the comment and the line containing it will be removed
	 * 
	 * ### Example Code:
	 * ```C++
	 * #include <iostream>
	 * using namespace std;
	 * 
	 * int main()
	 * {
	 * 	return 0;
	 * }
	 * ```
	 */

	/**
	 * @brief      My Child Class
	 *
	 * @tparam     T     My Template Parameter
	 */
	template<typename T>
	class ChildB: public BaseA
	{
		/**
		 * My Public Number
		 */
		int number;
		/**
		 * my private number
		 * any thing whether a type or a function prefixed with _ will not be included
		 */
		int _private_number;

		/**
		 * @brief      My Awesone abstract function
		 *
		 * @param[in]  a     { parameter_description }
		 * @param[in]  b     { parameter_description }
		 */
		void
		my_abstract_function(int a, int b) override {

		}

		void
		undocumented_function();

		/**
		 * @brief      My Private Function
		 * any thing whether a type or a function prefixed with _ will not be included
		 */
		void
		_my_private_function()
		{

		}
	};
}