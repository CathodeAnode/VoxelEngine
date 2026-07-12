function(ve_add_module_sources target source_dir)
    cmake_parse_arguments(VE_MOD "" "" "EXCLUDE" ${ARGN})
 
    file(GLOB CPPM_FILES CONFIGURE_DEPENDS "${source_dir}/*.cppm")
    file(GLOB CPP_FILES  CONFIGURE_DEPENDS "${source_dir}/*.cpp")
    file(GLOB C_FILES    CONFIGURE_DEPENDS "${source_dir}/*.c")
    file(GLOB H_FILES    CONFIGURE_DEPENDS "${source_dir}/*.h")
 
    if(VE_MOD_EXCLUDE)
        list(REMOVE_ITEM CPPM_FILES ${VE_MOD_EXCLUDE})
        list(REMOVE_ITEM CPP_FILES  ${VE_MOD_EXCLUDE})
        list(REMOVE_ITEM C_FILES    ${VE_MOD_EXCLUDE})
        list(REMOVE_ITEM H_FILES    ${VE_MOD_EXCLUDE})
    endif()
 
    if(CPPM_FILES)
        target_sources(${target}
            PUBLIC
                FILE_SET CXX_MODULES
                BASE_DIRS ${source_dir}
                FILES ${CPPM_FILES}
        )
    endif()
 
    if(CPP_FILES)
        target_sources(${target}
            PUBLIC
                FILE_SET CXX_MODULES
                BASE_DIRS ${source_dir}
                FILES ${CPP_FILES}
        )
    endif()
 
    if(C_FILES)
        target_sources(${target} PRIVATE ${C_FILES})
    endif()
 
    # FILE_SET HEADERS is PUBLIC here so Tests (or anything else linking
    # against this target) can see the header set too. If this target is
    # a pure executable with nothing linking against it, PRIVATE works
    # just as well -- the visibility only matters for consumers.
    if(H_FILES)
        target_sources(${target}
            PUBLIC
                FILE_SET HEADERS
                BASE_DIRS ${source_dir}
                FILES ${H_FILES}
        )
    endif()
endfunction()