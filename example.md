# Funciton `min`
```C++
template<typename T, typename TCompare>
inline static const T&
min(const T& a, const T& b, TCompare&& compare_func = TCompare());
```
 - **brief:**      Given two variables it will return the minimum value - **param[in]:**  a          The first value - **param[in]:**  b          The second value - **param[in]:**  compare_func  The compare function - **tparam:**     T          Type of the values - **tparam:**     TCompare   Type of the compare function - **return:**     The minimum value of the two
# Struct `BaseA`
```C++
struct BaseA;
```
 - **brief:**      This is my BaseA
## Funciton `my_abstract_function`
```C++
virtual void
my_abstract_function(int a,
					 int b) = 0;
```
 - **brief:**      This is my abstract function - **param[in]:**  a     Some random number - **param[in]:**  b     Some other random number
 ## Markdown Section This is a section that will be included in the output you can write anything markdown and it will be included but you have to put the   tag anywhere in the comment and the line containing it will be removed  ### Example Code: ```C++ #include <iostream> using namespace std;  int main() { 	return 0; } ```
# Struct `ChildB`
```C++
template<typename T>
class ChildB: public BaseA;
```
 - **brief:**      My Child Class - **tparam:**     T     My Template Parameter
## Variable `number`
```C++
int number;
```
 My Public Number
## Funciton `my_abstract_function`
```C++
void
my_abstract_function(int a, int b) override ;
```
 - **brief:**      My Awesone abstract function - **param[in]:**  a     { parameter_description } - **param[in]:**  b     { parameter_description }
