# Muratet (Spring supplied CMake build file) ---

ADD_SUBDIRECTORY(traces)

SET(ppSources
		"PP_Error"
		"PP_Supplier"
	)

ADD_LIBRARY(pp-supplier STATIC EXCLUDE_FROM_ALL ${ppSources})
SET_TARGET_PROPERTIES(pp-supplier PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")

# ---
