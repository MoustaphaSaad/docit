#Struct `BaseA`
```C++
struct BaseA;
```
 - **brief:**      This is my BaseA
##Funciton `my_abstract_function`
```C++
virtual void
my_abstract_function(int a,
					 int b) = 0;
```
 - **brief:**      This is my abstract function
 ##Markdown Section
#Struct `ChildB`
```C++
template<typename T>
class ChildB: public BaseA;
```
 - **brief:**      My Child Class
##Variable `number`
```C++
int number;
```
 My Public Number
##Funciton `my_abstract_function`
```C++
void
my_abstract_function(int a, int b) override ;
```
 - **brief:**      My Awesone abstract function