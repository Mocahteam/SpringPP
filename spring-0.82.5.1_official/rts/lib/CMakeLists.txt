
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/rts)

ADD_SUBDIRECTORY(lua)
ADD_SUBDIRECTORY(7z)
ADD_SUBDIRECTORY(oscpack)
ADD_SUBDIRECTORY(minizip)
ADD_SUBDIRECTORY(streflop)
ADD_SUBDIRECTORY(md5)
ADD_SUBDIRECTORY(lobby)
ADD_SUBDIRECTORY(headlessStubs)
# Muratet (Add P&P subdirectory) ---
ADD_SUBDIRECTORY(pp)
# ---
# Meresse (Add traces subdirectory in pp) ---
ADD_SUBDIRECTORY(pp/traces)
# ---

if    (NOT HEADLESS_SYSTEM)
	Add_Subdirectory(gml)
endif (NOT HEADLESS_SYSTEM)
