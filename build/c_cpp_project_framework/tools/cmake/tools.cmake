
# add prefix for all list members
# uage: prepend(out_var prefix in_var)
function(prepend out prefix)
    set(listVar "")
    foreach(f ${ARGN})
        list(APPEND listVar "${prefix}${f}")
    endforeach(f)
    set(${out} "${listVar}" PARENT_SCOPE)
endfunction()

# convert all members of list to absolute path(relative to CMAKE_CURRENT_SOURCE_DIR)
# usage: abspath(out_var list_var)
function(abspath out)
    set(listVar "")
    foreach(f ${ARGN})
        list(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
    endforeach(f)
    set(${out} "${listVar}" PARENT_SCOPE)
endfunction()


function(append_srcs_dir out_var)
    set(listVar ${${out_var}})
    foreach(f ${ARGN})
        aux_source_directory(${f} tmp)
        list(APPEND listVar ${tmp})
    endforeach(f)
    set(${out_var} "${listVar}" PARENT_SCOPE)
endfunction()

