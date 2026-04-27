#ifndef EBOOK_READER_BOOK_READER_HPP
#define EBOOK_READER_BOOK_READER_HPP

#include <mupdf/pdf.h>
#include <string>
#include "page_layout.hpp"
#include "app.h"
struct SDL_Texture;

typedef enum {
    BookPageLayoutPortrait,
    BookPageLayoutLandscape
} BookPageLayout;

class BookReader {
    public:
        BookReader(App& app, const char *path, int *result);
        ~BookReader();

        bool showUI = false;
        bool requestExit = false;

        void previous_page(int n);
        void next_page(int n);
        void zoom_in();
        void zoom_out();
        void move_page_up();
        void move_page_down();
        void move_page_left();
        void move_page_right();
        void pan_page(float dx, float dy);
        void zoom_by(float factor);
        void reset_page();
        void switch_page_layout();
        void draw();
        void rerender_page() { if (layout) layout->force_rerender(); }
    
        BookPageLayout currentPageLayout() {
            return _currentPageLayout;
        }
    
    private:
        void switch_current_page_layout(BookPageLayout bookPageLayout, int current_page);
        void load_icons();
        void free_icons();
    
        App& app_;
        fz_document *doc = NULL;
    
        BookPageLayout _currentPageLayout = BookPageLayoutPortrait;
        PageLayout *layout = NULL;
    
        std::string book_name;

        SDL_Texture* icon_back      = nullptr;
        SDL_Texture* icon_text      = nullptr;
        SDL_Texture* icon_toc       = nullptr;
        SDL_Texture* icon_bookmark  = nullptr;
        SDL_Texture* icon_search    = nullptr;
        SDL_Texture* icon_more      = nullptr;
        SDL_Texture* icon_portrait  = nullptr;
        SDL_Texture* icon_landscape = nullptr;
};

#endif
