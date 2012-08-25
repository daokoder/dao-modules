option(DAO_MODULES_GRAPHICS "Build the \"graphics\" module which adds threadsafe datastructures to Dao." ON)

set(DAO_GRAPHICS_SOURCE
		dao_graphics.c
		dao_geometry.c
		dao_path.c
		dao_font.c
		dao_opengl.c
		dao_glut.c
   )

if(DAO_MODULES_GRAPHICS)
	find_package(OpenGL REQUIRED)
	find_package(GLUT REQUIRED)

	include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
	message( "${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS}" )
    add_library(dao_graphics SHARED ${DAO_GRAPHICS_SOURCE})
    target_link_libraries(dao_graphics ${DAOLIB_TO_LINK})
	target_link_libraries(dao_graphics ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
    install(TARGETS dao_graphics DESTINATION ${DAO_MODULES_INSTALL_DIR})
endif(DAO_MODULES_GRAPHICS)
