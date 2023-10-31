#include "camera.h"

int test_list_all_camera_devices()
{
    int ret = 0;
    camera_list* list = list_all_camera_devices();
    if (!list)
    {
        ret = 1;
    }
    if (!ret && list->count > 0)
    {
        
    }
    return ret;
}

int main(int argc, char* argv[argc])
{
    if (argc > 0)
    {
        for (int i = 0; i < argc; ++i)
        {
            if (strcmp(argv[i], "test_list_all_camera_devices") == 0)
            {
                return test_list_all_camera_devices();
            }
        }
    }
    else
    {
        printf("Please specify which test to run.\n");
    }
    return 0;
}