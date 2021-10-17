#pragma once

#include <iostream>

extern bool vprint_verbose;

namespace
{
template <typename... T>
void vprint(T&... args)
{
	if (vprint_verbose) {
		(std::cout << ... << std::forward<T>(args));
	}
}
} // namespace
