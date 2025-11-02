#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include "simPeaje.h"

struct Rect {
    float x1;
    float y1;
    float x2;
    float y2;
};

enum class EstadoApp {
    Menu,
    Simulando
};

bool puntoDentro(const Rect& r, float px, float py) {
    if (px >= r.x1 && px <= r.x2 && py >= r.y1 && py <= r.y2) {
        return true;
    }
    return false;
}

void dibujarBoton(const Rect& r, const char* texto, ALLEGRO_FONT* font, bool hover) {
    ALLEGRO_COLOR fondo = hover ? al_map_rgb(70, 90, 140) : al_map_rgb(60, 70, 110);
    ALLEGRO_COLOR borde = al_map_rgb(200, 200, 220);
    ALLEGRO_COLOR blanco = al_map_rgb(240, 240, 255);
    al_draw_filled_rectangle(r.x1, r.y1, r.x2, r.y2, fondo);
    al_draw_rectangle(r.x1, r.y1, r.x2, r.y2, borde, 3.0f);
    float cx = (r.x1 + r.x2) * 0.5f;
    float cy = (r.y1 + r.y2) * 0.5f;
    int w = al_get_text_width(font, texto);
    int h = al_get_font_line_height(font);
    al_draw_text(font, blanco, cx - w * 0.5f, cy - h * 0.5f, 0, texto);
}

int main() {
    if (!al_init()) {
        return 1;
    }
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_image_addon();
    if (!al_install_mouse()) {
        return 1;
    }

    const int RESX = 1350;
    const int RESY = 900;
    ALLEGRO_DISPLAY* display = al_create_display(RESX, RESY);
    if (!display) {
        return 1;
    }
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    if (!queue) {
        al_destroy_display(display);
        return 1;
    }
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    if (!timer) {
        al_destroy_event_queue(queue);
        al_destroy_display(display);
        return 1;
    }
    ALLEGRO_FONT* font = al_create_builtin_font();
    if (!font) {
        al_destroy_timer(timer);
        al_destroy_event_queue(queue);
        al_destroy_display(display);
        return 1;
    }

    ALLEGRO_BITMAP* bmpCarro = al_load_bitmap("carro.png");
    if (!bmpCarro) {
        al_destroy_font(font);
        al_destroy_timer(timer);
        al_destroy_event_queue(queue);
        al_destroy_display(display);
        return 1;
    }
    const int srcW = al_get_bitmap_width(bmpCarro);
    const int srcH = al_get_bitmap_height(bmpCarro);

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_start_timer(timer);

    EstadoApp estado = EstadoApp::Menu;
    Config cfg;
    int carriles = cfg.numCarriles;
    int cabinas = cfg.numCabinas;
    double servicioMin = cfg.servicioMinSeg;
    double servicioMax = cfg.servicioMaxSeg;
    Simulador* simPtr = nullptr;

    Rect panel = { 150, 90, 1200, 780 };
    const float fila1Y = 240.0f;
    const float fila2Y = 360.0f;
    const float fila3Y = 480.0f;
    const float fila4Y = 600.0f;
    const float labelX = 220.0f;
    const float menosX = 680.0f;
    const float masX = 920.0f;
    const float valX = 815.0f;
    const float btnW = 110.0f;
    const float btnH = 70.0f;

    Rect menosCarril = { menosX, fila1Y - btnH * 0.5f, menosX + btnW, fila1Y + btnH * 0.5f };
    Rect masCarril = { masX, fila1Y - btnH * 0.5f, masX + btnW, fila1Y + btnH * 0.5f };
    Rect menosCab = { menosX, fila2Y - btnH * 0.5f, menosX + btnW, fila2Y + btnH * 0.5f };
    Rect masCab = { masX, fila2Y - btnH * 0.5f, masX + btnW, fila2Y + btnH * 0.5f };
    Rect menosMin = { menosX, fila3Y - btnH * 0.5f, menosX + btnW, fila3Y + btnH * 0.5f };
    Rect masMin = { masX, fila3Y - btnH * 0.5f, masX + btnW, fila3Y + btnH * 0.5f };
    Rect menosMax = { menosX, fila4Y - btnH * 0.5f, menosX + btnW, fila4Y + btnH * 0.5f };
    Rect masMax = { masX, fila4Y - btnH * 0.5f, masX + btnW, fila4Y + btnH * 0.5f };
    Rect aceptarBtn = { 980, 710, 1180, 790 };

    bool hoverMenosCarril = false;
    bool hoverMasCarril = false;
    bool hoverMenosCab = false;
    bool hoverMasCab = false;
    bool hoverMenosMin = false;
    bool hoverMasMin = false;
    bool hoverMenosMax = false;
    bool hoverMasMax = false;
    bool hoverAceptar = false;
    bool running = true;

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        }

        if (estado == EstadoApp::Menu) {
            if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
                float mx = ev.mouse.x;
                float my = ev.mouse.y;
                hoverMenosCarril = puntoDentro(menosCarril, mx, my);
                hoverMasCarril = puntoDentro(masCarril, mx, my);
                hoverMenosCab = puntoDentro(menosCab, mx, my);
                hoverMasCab = puntoDentro(masCab, mx, my);
                hoverMenosMin = puntoDentro(menosMin, mx, my);
                hoverMasMin = puntoDentro(masMin, mx, my);
                hoverMenosMax = puntoDentro(menosMax, mx, my);
                hoverMasMax = puntoDentro(masMax, mx, my);
                hoverAceptar = puntoDentro(aceptarBtn, mx, my);
            }
            if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                float mx = ev.mouse.x;
                float my = ev.mouse.y;
                if (puntoDentro(menosCarril, mx, my)) {
                    carriles = carriles - 1;
                    if (carriles < 1) {
                        carriles = 1;
                    }
                }
                if (puntoDentro(masCarril, mx, my)) {
                    carriles = carriles + 1;
                }
                if (puntoDentro(menosCab, mx, my)) {
                    cabinas = cabinas - 1;
                    if (cabinas < 1) {
                        cabinas = 1;
                    }
                }
                if (puntoDentro(masCab, mx, my)) {
                    cabinas = cabinas + 1;
                }
                if (puntoDentro(menosMin, mx, my)) {
                    servicioMin = servicioMin - 0.5;
                    if (servicioMin < 0.5) {
                        servicioMin = 0.5;
                    }
                    if (servicioMin > servicioMax) {
                        servicioMin = servicioMax;
                    }
                }
                if (puntoDentro(masMin, mx, my)) {
                    servicioMin = servicioMin + 0.5;
                    if (servicioMin > servicioMax) {
                        servicioMin = servicioMax;
                    }
                }
                if (puntoDentro(menosMax, mx, my)) {
                    servicioMax = servicioMax - 0.5;
                    if (servicioMax < servicioMin) {
                        servicioMax = servicioMin;
                    }
                }
                if (puntoDentro(masMax, mx, my)) {
                    servicioMax = servicioMax + 0.5;
                    if (servicioMax > 30.0) {
                        servicioMax = 30.0;
                    }
                }
                if (puntoDentro(aceptarBtn, mx, my)) {
                    cfg.numCarriles = carriles;
                    cfg.numCabinas = cabinas;
                    cfg.servicioMinSeg = servicioMin;
                    cfg.servicioMaxSeg = servicioMax;
                    cfg.anchoPantalla = RESX;
                    cfg.altoPantalla = RESY;
                    simPtr = new Simulador(cfg);
                    estado = EstadoApp::Simulando;
                }
            }
            if (ev.type == ALLEGRO_EVENT_TIMER) {
                al_clear_to_color(al_map_rgb(18, 22, 30));
                ALLEGRO_COLOR marco = al_map_rgb(180, 190, 210);
                ALLEGRO_COLOR titulo = al_map_rgb(235, 235, 245);
                ALLEGRO_COLOR texto = al_map_rgb(220, 225, 235);
                al_draw_rectangle(panel.x1, panel.y1, panel.x2, panel.y2, marco, 3.0f);
                al_draw_text(font, titulo, panel.x1 + 40.0f, panel.y1 + 30.0f, 0, "Configuracion de Simulacion");
                al_draw_text(font, texto, labelX, fila1Y - 10.0f, 0, "Carriles:");
                dibujarBoton(menosCarril, "-", font, hoverMenosCarril);
                dibujarBoton(masCarril, "+", font, hoverMasCarril);
                al_draw_textf(font, texto, valX, fila1Y - 10.0f, 0, "%d", carriles);
                al_draw_text(font, texto, labelX, fila2Y - 10.0f, 0, "Peajes (cabinas):");
                dibujarBoton(menosCab, "-", font, hoverMenosCab);
                dibujarBoton(masCab, "+", font, hoverMasCab);
                al_draw_textf(font, texto, valX, fila2Y - 10.0f, 0, "%d", cabinas);
                al_draw_text(font, texto, labelX, fila3Y - 10.0f, 0, "Servicio minimo (s):");
                dibujarBoton(menosMin, "-", font, hoverMenosMin);
                dibujarBoton(masMin, "+", font, hoverMasMin);
                al_draw_textf(font, texto, valX, fila3Y - 10.0f, 0, "%.1f", servicioMin);
                al_draw_text(font, texto, labelX, fila4Y - 10.0f, 0, "Servicio maximo (s):");
                dibujarBoton(menosMax, "-", font, hoverMenosMax);
                dibujarBoton(masMax, "+", font, hoverMasMax);
                al_draw_textf(font, texto, valX, fila4Y - 10.0f, 0, "%.1f", servicioMax);
                dibujarBoton(aceptarBtn, "Aceptar", font, hoverAceptar);
                al_flip_display();
            }
        }
        else if (estado == EstadoApp::Simulando) {
            if (ev.type == ALLEGRO_EVENT_TIMER) {
                if (simPtr != nullptr) {
                    simPtr->actualizar();
                }
                al_clear_to_color(al_map_rgb(20, 20, 30));
                for (int i = 0; i < cfg.numCarriles; i++) {
                    float x = static_cast<float>(cfg.carrilInicioX + i * cfg.sepCarrilX);
                    al_draw_line(x, 0.0f, x, static_cast<float>(cfg.altoPantalla), al_map_rgb(80, 80, 120), 2.0f);
                }
                al_draw_line(0.0f, static_cast<float>(cfg.posPeajeY), static_cast<float>(cfg.anchoPantalla), static_cast<float>(cfg.posPeajeY), al_map_rgb(200, 200, 50), 2.0f);
                if (simPtr != nullptr) {
                    const std::vector<Vehiculo>& vehiculos = simPtr->getVehiculos();
                    for (const auto& v : vehiculos) {
                        float x1 = static_cast<float>(v.x);
                        float y1 = static_cast<float>(v.y);
                        float w = static_cast<float>(v.ancho);
                        float h = static_cast<float>(v.largo);
                        al_draw_scaled_bitmap(bmpCarro, 0, 0, srcW, srcH, x1, y1, w, h, 0);
                    }
                    int atend = simPtr->getAtendidos();
                    al_draw_textf(font, al_map_rgb(230, 230, 230), static_cast<float>(RESX - 20), 20.0f, ALLEGRO_ALIGN_RIGHT, "Atendidos: %d", atend);
                }
                al_flip_display();
            }
        }
    }

    if (simPtr != nullptr) {
        delete simPtr;
        simPtr = nullptr;
    }
    al_destroy_bitmap(bmpCarro);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    return 0;
}

