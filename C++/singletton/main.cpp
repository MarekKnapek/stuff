#include "singletton1.h"
#include "singletton2.h"
#include "singletton3.h"
#include "singletton4.h"

#include <iostream>
#include <thread>
#include <vector>

int main2();

int main()
{
	::std::cout << "=== START main ===" << ::std::endl;
	void(*func_arr[])(void) =
	{
		[](){
			for(int i = 0; i != 10000; ++i){
				::mk::singletton1::get_instance();
			}
		},
		[](){
			for(int i = 0; i != 10000; ++i){
				::mk::singletton2::get_instance();
			}
		},
		[](){
			for(int i = 0; i != 10000; ++i){
				::mk::singletton3::get_instance();
			}
		},
		[](){
			for(int i = 0; i != 10000; ++i){
				::mk::singletton4::get_instance();
			}
		}
	};
	unsigned n = 10;
	::std::vector<::std::thread> vec;
	vec.resize(n);
	unsigned repeat = 10;
	for(unsigned r = 0; r != repeat; ++r){
		for(unsigned i = 0; i != n; ++i){
			vec[i] = ::std::thread(func_arr[i % 4]);
		}
		for(unsigned i = 0; i != n; ++i){
			vec[i].join();
		}
	}
	::std::cout << "=== END main ===" << ::std::endl;
	main2();
}
