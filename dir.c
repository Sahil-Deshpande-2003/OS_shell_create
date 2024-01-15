#include <stdio.h>
#include <dirent.h>

int isDirectoryExists(const char *path) {
    DIR *dir = opendir(path);
    
    if (dir != NULL) {
        closedir(dir);
        return 1;  // Directory exists
    } else {
        return 0;  // Directory does not exist
    }
}

int main() {
    const char *directoryPath = "/home/sahil/Video";

    if (isDirectoryExists(directoryPath)) {
        printf("Directory exists.\n");
    } else {
        printf("Directory does not exist.\n");
    }

    return 0;
}

