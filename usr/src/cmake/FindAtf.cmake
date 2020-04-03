include (CheckFunctionExists)

check_function_exists(kqueue SYSTEM_Atf)

if(SYSTEM_Atf)
	message("Using system built-in Kernel Queue")
	set(Atf_FOUND TRUE)
else()
	message("Searching for libkqueue")
	find_path(Atf_INCLUDE_DIR sys/event.h
		PATHS
		"/usr/include"
		"/usr/local/include"
		PATH_SUFFIXES "kqueue"
		)

	find_library(Atf_LIBRARY
		NAMES libatf-c.so
		PATHS /usr/lib /usr/local/lib
		)

	if (Atf_INCLUDE_DIR AND Atf_LIBRARY)
		set(Atf_FOUND TRUE)
		set(Atf_LIBRARIES ${Atf_LIBRARY})
		set(Atf_INCLUDE_DIRS ${Atf_INCLUDE_DIR})
		find_package_handle_standard_args(Atf DEFAULT_MSG
			Atf_LIBRARIES
			Atf_INCLUDE_DIRS)
	else (Atf_INCLUDE_DIR AND Atf_LIBRARY)
		set(Atf_FOUND FALSE)
	endif (Atf_INCLUDE_DIR AND Atf_LIBRARY)
endif()

if(Atf_FOUND AND NOT TARGET Atf::Atf)
	if (SYSTEM_Atf)
		add_library(Atf::Atf IMPORTED INTERFACE)
	else()
		add_library(Atf::Atf UNKNOWN IMPORTED)
		set_property(TARGET Atf::Atf APPEND PROPERTY
			IMPORTED_LOCATION "${Atf_LIBRARY}")
	endif()

	set_target_properties(Atf::Atf PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${Atf_INCLUDE_DIRS}")
endif()