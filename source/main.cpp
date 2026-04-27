#include "app.h"
#include "menu_chooser.h"
#include "menu_book_reader.h"

extern "C" {
    #include "logger.h"
}

int main(int argc, char *argv[]) {
    App app;
    if (!app.init())
        return 1;

    if (argc == 2) {
        LOG_I("Opening book from association: %s", argv[1]);
        Menu_OpenBook(app, argv[1]);
    } else {
        LOG_I("Starting book chooser...");
        Menu_StartChoosing(app);
    }

    return 0;
}
