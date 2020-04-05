include (CheckFunctionExists)

check_function_exists(kqueue SYSTEM_Kqueue)

if(SYSTEM_Kqueue)
	message("Using system built-in Kernel Queue")
	set(Kqueue_FOUND TRUE)
else()
	message("Searching for libkqueue")

	pkg_check_modules(Kqueue IMPORTED_TARGET kqueue)

	if(NOT Kqueue_FOUND)
		find_path(Kqueue_INCLUDE_DIR sys/event.h
			PATHS
			"/usr/include"
			"/usr/local/include"
			PATH_SUFFIXES "kqueue"
			)

		find_library(Kqueue_LIBRARY
			NAMES libkqueue.so
			PATHS /usr/lib /usr/local/lib
			)

		if (Kqueue_INCLUDE_DIR AND Kqueue_LIBRARY)
			set(Kqueue_FOUND TRUE)
			set(Kqueue_LIBRARIES ${Kqueue_LIBRARY})
			set(Kqueue_INCLUDE_DIRS ${Kqueue_INCLUDE_DIR})
		else ()
			message("Libkqueue not found")
			set(Kqueue_FOUND FALSE)
		endif (Kqueue_INCLUDE_DIR AND Kqueue_LIBRARY)
	endif()

	find_package_handle_standard_args(Kqueue DEFAULT_MSG
		Kqueue_LIBRARIES
		Kqueue_INCLUDE_DIRS)
endif()

if(Kqueue_FOUND AND NOT TARGET Kqueue::Kqueue)
	if (SYSTEM_Kqueue)
		add_library(Kqueue::Kqueue IMPORTED INTERFACE)
	else()
		add_library(Kqueue::Kqueue UNKNOWN IMPORTED)
		set_property(TARGET Kqueue::Kqueue APPEND PROPERTY
			IMPORTED_LOCATION "${Kqueue_LIBRARY}")
	endif()

	set_target_properties(Kqueue::Kqueue PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${Kqueue_INCLUDE_DIRS}")
endif()