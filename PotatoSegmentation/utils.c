#include <stdio.h>
#include <windows.h>

void list_files_in_current_dir() {
    WIN32_FIND_DATAA findData;

    HANDLE hFind = INVALID_HANDLE_VALUE;

    hFind = FindFirstFileA(".\\*", &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error: Cannot find folder.\n");
        return;
    }

    printf("--- File List ---\n");

    do {
        printf("%s\n", findData.cFileName);
    } while (FindNextFileA(hFind, &findData) != 0);

    printf("--------------------------\n");

    FindClose(hFind);
}