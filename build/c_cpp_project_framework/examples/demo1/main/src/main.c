
#include <stdio.h>
#include "test.h"
// #include "global_config.h"
// #include "global_build_info_time.h"
// #include "global_build_info_version.h"

// #include "lib1.h"

// #if CONFIG_COMPONENT2_ENABLED
// #include "lib2.h"
// #endif

// #if CONFIG_COMPONENT3_ENABLED
// #include "lib3.h"
// #endif


// #include "lib2_private.h"  // We can't include lib2_private.h for it's compoent2's private include dir


int main()
{
    foo();
    test_libcollections();

    test_liblog();




    return 0;
}

