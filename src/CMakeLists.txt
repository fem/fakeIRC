#add_libary(fakeirc-common STATIC
#)

add_executable(fakeirc
	main.cpp
	vprint.cpp
	ConnectionHandler.cpp
	ClientHandler.cpp
)
target_compile_features(fakeirc
	PUBLIC
		cxx_std_17
)

