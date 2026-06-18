#include "pacman.h"
#include <mmsystem.h>

#ifdef _WIN32

#include <mmsystem.h>

void play_hogwarts_music(void) {
    PlaySound(
        TEXT("serpents_waltz.wav"),
        NULL,
        SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT
    );
}



void stop_hogwarts_music(void) {
    PlaySound(NULL, NULL, 0);
}

#else

void play_hogwarts_music(void){}
void stop_hogwarts_music(void){}

#endif
/*
 * ============================================================
 *  VIEW — Renderização ANSI nível arcade Nintendo
 * ============================================================
 *  Reescrito do zero para qualidade profissional. Tudo em C
 *  com ANSI puro (sem ncurses, sem dependência externa):
 *
 *  - Paredes com auto-tiling (cantos arredondados, T, blocos)
 *  - Pac-Man com 4 frames de animação e boca direcional
 *  - Fantasmas com indicador de direção, piscar branco no fim
 *  - Power pellet com respiração (radiance pulse)
 *  - Splash arcade com logo gigante e "Pressione ENTER"
 *  - Menu redesenhado com sombra 3D e seletor de mapas
 *  - HUD lateral em estilo arcade cabinet com:
 *      - Gauge gráfico do power-up (barra que diminui)
 *      - Combo grande, cards dos fantasmas com IA
 *      - Estruturas em tempo real (BST/AVL/Graph)
 *      - Barra de progresso de pellets
 *  - Bonus floats (+200, +400...) sobem suavemente
 *  - Animação de morte estendida (9 frames)
 *  - Tela de vitória com flash + benchmark de sorts
 *  - High Scores com pódio para top 3
 * ============================================================
 */

/* ======================== PLATAFORMA ======================== */

#ifndef _WIN32
static struct termios orig_termios;
#endif

void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void enable_ansi(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
    SetConsoleOutputCP(65001);
#endif
}

int kbhit_custom(void) {
#ifdef _WIN32
    return _kbhit();
#else
    int ch = getchar();
    if (ch != EOF) { ungetc(ch, stdin); return 1; }
    return 0;
#endif
}

int getch_custom(void) {
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}

int manhattan(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

/* ======================== TERMINAL ======================== */

static void move_cursor(int x, int y) { printf("\033[%d;%dH", y + 1, x + 1); }
static void hide_cursor(void)         { printf("\033[?25l"); }
static void show_cursor(void)         { printf("\033[?25h"); }
static void reset_cursor(void)        { printf("\033[H"); }

void view_clear(void) { printf("\033[2J\033[H"); }
void view_flush(void) { fflush(stdout); }
//-----------------------------------------------
void view_init(void) {
    play_hogwarts_music();

    enable_ansi();
    setvbuf(stdout, NULL, _IOFBF, 1 << 20);
#ifndef _WIN32
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
#endif
    hide_cursor();
    printf("\033[40m"); /* fundo preto */
}
//---------------------------------------
void view_cleanup(void) {

    stop_hogwarts_music();

    show_cursor();
    printf(CLR_RESET "\033[49m");
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
#endif
}

/* ======================== PRIMITIVAS ======================== */

static void print_abs(int col, int row, const char *color, const char *str) {
    move_cursor(col, row);
    printf("%s%s%s", color, str, CLR_RESET);
}

static void clear_rect(int col, int row, int w, int h) {
    for (int dy = 0; dy < h; dy++) {
        move_cursor(col, row + dy);
        for (int dx = 0; dx < w; dx++) printf(" ");
    }
}

/* ======================== AUTO-TILING DA PAREDE ======================== */
/*
 * O tile da parede é escolhido com base nos vizinhos cardeais.
 * Resultado: cantos arredondados, blocos sólidos e linhas — o
 * labirinto fica parecido com o gabinete original do Pac-Man,
 * em vez de "▓▓" repetido.
 */

static int is_wall_or_door(GameModel *m, int x, int y) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return 1; /* fora = parede */
    int t = m->grid[y][x];
    return (t == TILE_WALL || t == TILE_DOOR);
}

/* ======================== ILUMINACAO ======================== */

static int candle_light_level(GameModel *m, int x, int y) {
    int light = 2; /* mapa normal */

    int dx = abs(x - m->pacman.x);
    int dy = abs(y - m->pacman.y);
    int d = dx + dy;

    if (d <= 2)
        light = 4;
    else if (d <= 5)
        light = 3;

    return light;
}


/* ======================== PAREDES ======================== */

static const char *wall_glyph(GameModel *m, int x, int y) {
    int u = is_wall_or_door(m, x, y - 1);
    int d = is_wall_or_door(m, x, y + 1);
    int l = is_wall_or_door(m, x - 1, y);
    int r = is_wall_or_door(m, x + 1, y);

    /* Núcleo */
if (u && d && l && r) {
    int tex = (x * 7 + y * 13) % 4;

    if (tex == 0) return "▓▓";
    if (tex == 1) return "▒▓";
    if (tex == 2) return "▓▒";
    return "██";
}

/* T-junções */
if (u && d && (l || r)) {
    int tex = (x * 7 + y * 13) % 4;

    if (tex == 0) return "▓▓";
    if (tex == 1) return "▒▓";
    if (tex == 2) return "▓▒";
    return "██";
}

if (l && r && (u || d)) {
    int tex = (x * 7 + y * 13) % 4;

    if (tex == 0) return "▓▓";
    if (tex == 1) return "▒▓";
    if (tex == 2) return "▓▒";
    return "██";
}

    /* Cantos arredondados (estilo arcade) */
    if (d && r && !u && !l) return "╭─";
    if (d && l && !u && !r) return "─╮";
    if (u && r && !d && !l) return "╰─";
    if (u && l && !d && !r) return "─╯";

    /* Linhas finas isoladas */
    if (l && r) return "──";
    if (u && d) return "││";

    /* Ponta única */
    if (l)         return "─ ";
    if (r)         return " ─";
    if (u || d)    return "││";

   int tex = (x * 7 + y * 13) % 4;

if (tex == 0) return "▓▓";
if (tex == 1) return "▒▓";
if (tex == 2) return "▓▒";
return "██";
}

/* ======================== CÉLULAS DO MAPA ======================== */

void view_draw_cell(GameModel *m, int x, int y) {
    int tile = m->grid[y][x];

    if (m->debug_mode && tile != TILE_WALL && tile != TILE_DOOR) {
        int v = model_coord_to_vertex(x, y);
        if (m->bfs_dist[v] >= 0 && m->bfs_dist[v] < 100) {
            move_cursor(OFFSET_X + x * CELL_W, OFFSET_Y + y);
            printf("%s%02d%s", CLR_DIM, m->bfs_dist[v] % 100, CLR_RESET);
            return;
        }
    }

    int sx = OFFSET_X + x * CELL_W;
    int sy = OFFSET_Y + y;
    move_cursor(sx, sy);

    int light = candle_light_level(m, x, y);

    switch (tile) {
        case TILE_WALL: {
    const char *g = wall_glyph(m, x, y);
    const char *wall_color;

    if (light >= 4) {
        wall_color = "\033[38;5;252m";
        g = "██";
    } else if (light == 3) {
        wall_color = "\033[38;5;248m";
        g = "▓▓";
    } else {
        wall_color = "\033[38;5;240m";
    }

    printf("%s%s%s", wall_color, g, CLR_RESET);
    break;
}

        case TILE_PELLET:
    if (light >= 3)
        printf("%s •%s", CLR_PELLET, CLR_RESET);
    else
        printf("\033[38;5;244m •%s", CLR_RESET);
    break;

        case TILE_POWER: {
            if (light <= 0) {
                printf("[38;5;232m  %s", CLR_RESET);
                break;
            }

            int ph = (m->frame_count / 3) % 8;
            const char *glyph;
            const char *color;

            if      (ph < 2) { glyph = " ·"; color = CLR_POWER; }
            else if (ph < 4) { glyph = " ✦"; color = CLR_POWER; }
            else if (ph < 6) { glyph = " ⚡"; color = CLR_ORANGE; }
            else             { glyph = " ✦"; color = CLR_POWER; }

            printf("%s%s%s", color, glyph, CLR_RESET);
            break;
        }

        case TILE_DOOR:
            if (light <= 0)
                printf("[38;5;232m  %s", CLR_RESET);
            else if (light == 1)
                printf("[38;5;236m══%s", CLR_RESET);
            else
                printf("%s══%s", CLR_DOOR, CLR_RESET);
            break;

        default:
            if (light <= 0)
                printf("[48;5;232m  %s", CLR_RESET);
            else if (light == 1)
                printf("[48;5;233m  %s", CLR_RESET);
            else
                printf("  ");
            break;
    }
}

void view_draw_map(GameModel *m) {
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            view_draw_cell(m, x, y);

    /* Moldura externa em estilo arcade cabinet */
    int left   = OFFSET_X - 2;
    int top    = OFFSET_Y - 1;
    int right  = OFFSET_X + MAP_W * CELL_W;
    int bottom = OFFSET_Y + MAP_H;

    move_cursor(left, top);
    printf("%s╔", CLR_WALL_HI);
    for (int i = 0; i < MAP_W * CELL_W + 2; i++) printf("═");
    printf("╗%s", CLR_RESET);

    for (int row = top + 1; row < bottom + 1; row++) {
        move_cursor(left,      row);  printf("%s║%s", CLR_WALL_HI, CLR_RESET);
        move_cursor(right + 1, row);  printf("%s║%s", CLR_WALL_HI, CLR_RESET);
    }

    move_cursor(left, bottom + 1);
    printf("%s╚", CLR_WALL_HI);
    for (int i = 0; i < MAP_W * CELL_W + 2; i++) printf("═");
    printf("╝%s", CLR_RESET);
}

/* ======================== ENTIDADES — PAC-MAN ======================== */

static const char *pacman_sprite(GameModel *m) {
    int anim = (m->frame_count / 3) % 4;
    int dx = m->current_dx, dy = m->current_dy;
    if (dx == 0 && dy == 0) dx = 1; /* parado: olha p/ direita */

    if (anim == 0) return "●●";

    if (anim == 1 || anim == 3) {
        if (dx > 0)  return "ᗧ•";
        if (dx < 0)  return "•ᗤ";
        if (dy < 0)  return "ᗢ ";
        return "ᗣ ";
    }

    if (dx > 0)  return "ᗧ ";
    if (dx < 0)  return " ᗤ";
    if (dy < 0)  return "ᗢ ";
    return "ᗣ ";
}

/* ======================== ENTIDADES — FANTASMAS ======================== */

static const char *ghost_sprite(Ghost *g, int frame) {
    if (g->vulnerable) {
        return (frame % 6 < 3) ? "ᗣ▒" : "◌◌";
    }
    if (g->e.dx > 0)  return "ᗣ›";
    if (g->e.dx < 0)  return "‹ᗣ";
    if (g->e.dy < 0)  return "ᗣˆ";
    if (g->e.dy > 0)  return "ᗣˇ";
    return "ᗣ ";
}

static const char *ghost_color(int i, Ghost *g, int frame) {
    if (g->vulnerable) {
        return (frame % 8 < 4) ? CLR_VULN : CLR_VULN2;
    }
    switch (i) {
        case 0: return CLR_BLINKY;
        case 1: return CLR_PINKY;
        case 2: return CLR_INKY;
        case 3: return CLR_CLYDE;
    }
    return CLR_RESET;
}

void view_draw_entities(GameModel *m) {
    for (int i = 0; i < GHOSTS; i++) {
    Ghost *g = &m->ghosts[i];
        move_cursor(OFFSET_X + g->e.x * CELL_W, OFFSET_Y + g->e.y);
        printf("%s%s%s", ghost_color(i, g, m->frame_count),
               ghost_sprite(g, m->frame_count), CLR_RESET);
    }
    move_cursor(OFFSET_X + m->pacman.x * CELL_W, OFFSET_Y + m->pacman.y);
    printf("%s%s%s", CLR_PACMAN, pacman_sprite(m), CLR_RESET);
}

void view_erase_entities(GameModel *m) {
    view_draw_cell(m, m->pacman.x, m->pacman.y);

    for (int i = 0; i < GHOSTS; i++) {
        view_draw_cell(m, m->ghosts[i].e.x, m->ghosts[i].e.y);
    }
}

/* ======================== HEADER SUPERIOR ======================== */

static void view_draw_header(GameModel *m) {
    int center = OFFSET_X + MAP_W * CELL_W / 2;

    /* 1UP */
    move_cursor(OFFSET_X, 1);
    printf("%s 1UP %s", CLR_DIM, CLR_RESET);
    move_cursor(OFFSET_X + 1, 2);
    printf("%s%6d%s", CLR_SCORE, m->score, CLR_RESET);

    /* HIGH SCORE central */
    move_cursor(center - 6, 1);
    printf("%sHIGH SCORE%s", CLR_DIM, CLR_RESET);
    move_cursor(center - 3, 2);
    printf("%s%6d%s", CLR_TITLE, m->high_score, CLR_RESET);

    /* NIVEL à direita */
    int right = OFFSET_X + MAP_W * CELL_W - 8;
    move_cursor(right, 1);
    printf("%sNIVEL%s", CLR_DIM, CLR_RESET);
    move_cursor(right + 1, 2);
    printf("%s  %02d %s", CLR_CYAN, m->level, CLR_RESET);

    /* Separador fino */
    move_cursor(OFFSET_X - 2, 3);
    printf("%s", CLR_DIM2);
    for (int i = 0; i < MAP_W * CELL_W + 4; i++) printf("─");
    printf("%s", CLR_RESET);

    /* Linha de vidas + badge auto-play */
    move_cursor(OFFSET_X, 4);
    for (int i = 0; i < MAP_W * CELL_W; i++) printf(" ");
    move_cursor(OFFSET_X, 4);
    printf("%sVIDAS%s ", CLR_DIM, CLR_RESET);
    for (int i = 0; i < m->lives - 1 && i < 5; i++)
        printf("%sᗧ %s", CLR_PACMAN, CLR_RESET);

    if (m->auto_play) {
        move_cursor(center - 7, 4);
        int blink = (m->frame_count / 8) % 2;
        if (blink)
            printf("%s ▶ AUTO-PLAY ◀ %s", CLR_AUTO_TAG, CLR_RESET);
        else
            printf("%s   AUTO-PLAY   %s", CLR_GREEN, CLR_RESET);
    }
}

/* ======================== PAINEL LATERAL (arcade cabinet) ======================== */

#define PANEL_X  (OFFSET_X + MAP_W * CELL_W + 4)
#define PANEL_W  22

static void panel_top(int row) {
    move_cursor(PANEL_X, row);
    printf("%s╭", CLR_PANEL_BORDER);
    for (int i = 0; i < PANEL_W - 2; i++) printf("─");
    printf("╮%s", CLR_RESET);
}

static void panel_bottom(int row) {
    move_cursor(PANEL_X, row);
    printf("%s╰", CLR_PANEL_BORDER);
    for (int i = 0; i < PANEL_W - 2; i++) printf("─");
    printf("╯%s", CLR_RESET);
}

static void panel_sep(int row) {
    move_cursor(PANEL_X, row);
    printf("%s├", CLR_PANEL_BORDER);
    for (int i = 0; i < PANEL_W - 2; i++) printf("─");
    printf("┤%s", CLR_RESET);
}

static void panel_text(int row, const char *color, const char *text) {
    move_cursor(PANEL_X, row);
    printf("%s│%s", CLR_PANEL_BORDER, CLR_RESET);
    move_cursor(PANEL_X + 1, row);
    printf("                    "); /* limpa */
    move_cursor(PANEL_X + 2, row);
    printf("%s%s%s", color, text, CLR_RESET);
    move_cursor(PANEL_X + PANEL_W - 1, row);
    printf("%s│%s", CLR_PANEL_BORDER, CLR_RESET);
}

static void panel_header(int row, const char *text) {
    move_cursor(PANEL_X, row);
    printf("%s│%s", CLR_PANEL_BORDER, CLR_RESET);
    move_cursor(PANEL_X + 1, row);
    printf("%s%s", CLR_SEL_BG, CLR_PANEL_HDR);
    int len = (int)strlen(text);
    int pad = (PANEL_W - 2 - len) / 2;
    if (pad < 0) pad = 0;
    for (int i = 0; i < pad; i++) printf(" ");
    printf("%s", text);
    int rem = PANEL_W - 2 - pad - len;
    if (rem < 0) rem = 0;
    for (int i = 0; i < rem; i++) printf(" ");
    printf("%s", CLR_RESET);
    move_cursor(PANEL_X + PANEL_W - 1, row);
    printf("%s│%s", CLR_PANEL_BORDER, CLR_RESET);
}

/* Gauge horizontal (barra que vai diminuindo) */
static void panel_gauge(int row, const char *label, int value, int max, const char *color) {
    move_cursor(PANEL_X, row);
    printf("%s│%s ", CLR_PANEL_BORDER, CLR_RESET);
    printf("%s%-5s%s ", CLR_DIM, label, CLR_RESET);

    int bar_w = PANEL_W - 10;
    int filled = (max > 0) ? (value * bar_w) / max : 0;
    if (filled < 0)      filled = 0;
    if (filled > bar_w)  filled = bar_w;

    printf("%s", color);
    for (int i = 0; i < filled; i++)         printf("█");
    printf("%s", CLR_DIM2);
    for (int i = filled; i < bar_w; i++)     printf("░");
    printf("%s ", CLR_RESET);

    move_cursor(PANEL_X + PANEL_W - 1, row);
    printf("%s│%s", CLR_PANEL_BORDER, CLR_RESET);
}

/* Card de fantasma: cor + sprite + nome + algoritmo */
static void panel_ghost_card(int row, const char *clr, const char *name, const char *ai) {
    move_cursor(PANEL_X, row);
    printf("%s│%s ", CLR_PANEL_BORDER, CLR_RESET);
    printf("%sᗣ%s %s%-7s%s %s%-8s%s",
           clr, CLR_RESET,
           clr, name, CLR_RESET,
           CLR_DIM, ai, CLR_RESET);
    move_cursor(PANEL_X + PANEL_W - 1, row);
    printf("%s│%s", CLR_PANEL_BORDER, CLR_RESET);
}

void view_draw_side_panel(GameModel *m) {
    int r = OFFSET_Y - 1;

    panel_top(r++);
    panel_header(r++, "HOGWARTS MAZE");
    panel_sep(r++);

    /* ---- Power gauge ---- */
    panel_header(r++, "FEITICO POWER");
    if (m->power_timer > 0) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%2ds restante", m->power_timer / 12);
        panel_text(r++, CLR_POWER, buf);
        panel_gauge(r++, "GAUGE", m->power_timer, POWER_TIME, CLR_POWER);
    } else {
        panel_text(r++, CLR_DIM, "inativo");
        panel_gauge(r++, "GAUGE", 0, POWER_TIME, CLR_POWER);
    }
    panel_sep(r++);

    /* ---- Combo grande ---- */
    {
        panel_header(r++, "COMBO");
        char buf[48];
        if (m->ghost_eat_combo > 0)
            snprintf(buf, sizeof(buf), " x%d  +%d pts",
                     m->ghost_eat_combo, 200 << (m->ghost_eat_combo - 1));
        else
            snprintf(buf, sizeof(buf), " x0  (sem combo)");
        panel_text(r++, m->ghost_eat_combo > 0 ? CLR_ORANGE : CLR_DIM, buf);
    }
    panel_sep(r++);

    /* ---- Cards dos fantasmas ---- */
    panel_header(r++, "DEMENTADORES / IA");
    panel_ghost_card(r++, CLR_BLINKY, "Voldem.", "Dijkstra");
    panel_ghost_card(r++, CLR_PINKY,  "Bellatrix",  "BFS");
    panel_ghost_card(r++, CLR_INKY,   "Lucius",   "DFS");
    panel_ghost_card(r++, CLR_CLYDE,  "Dobby",  "Manhatt.");
    panel_sep(r++);

    /* ---- Estruturas ---- */
    panel_header(r++, "ESTRUTURAS");
    {
        char buf[48];
        snprintf(buf, sizeof(buf), "BST  h=%-2d  n=%-3d",
                 bst_height(m->score_tree), bst_count(m->score_tree));
        panel_text(r++, CLR_CYAN, buf);

        snprintf(buf, sizeof(buf), "AVL  h=%-2d  b=%-2d",
                 avl_height(m->powerup_tree), avl_balance_factor(m->powerup_tree));
        panel_text(r++, CLR_CYAN, buf);
        snprintf(buf, sizeof(buf), "Rot S=%-2d D=%-2d", avl_rotation_single_count(), avl_rotation_double_count());
        panel_text(r++, CLR_MAGENTA, buf);

        int edges = 0;
        if (m->maze_graph) {
            for (int i = 0; i < m->maze_graph->num_vertices; i++) {
                AdjNode *cur = m->maze_graph->adj_list[i];
                while (cur) { edges++; cur = cur->next; }
            }
        }
        snprintf(buf, sizeof(buf), "Graph  E=%-4d", edges);
        panel_text(r++, CLR_CYAN, buf);
    }
    panel_sep(r++);

    /* ---- DOTS ---- */
    panel_header(r++, "DOTS");
    panel_gauge(r++, "MAP",
                m->total_pellets - m->pellets_left,
                m->total_pellets > 0 ? m->total_pellets : 1,
                CLR_PELLET);
    {
        char buf[48];
        snprintf(buf, sizeof(buf), "restam: %d", m->pellets_left);
        panel_text(r++, CLR_PELLET, buf);
    }
    panel_sep(r++);

    /* ---- Auto-play stats ---- */
    if (m->auto_play) {
        panel_header(r++, "AUTO-PLAY");
        char buf[48];
        snprintf(buf, sizeof(buf), "pellets:  %3d", m->auto_pellets_eaten);
        panel_text(r++, CLR_AUTO_TAG, buf);
        snprintf(buf, sizeof(buf), "ghosts :  %3d", m->auto_ghosts_eaten);
        panel_text(r++, CLR_AUTO_TAG, buf);
        char st[16];
        strncpy(st, m->auto_status, 14); st[14] = '\0';
        snprintf(buf, sizeof(buf), "modo: %s", st);
        panel_text(r++, CLR_GREEN, buf);
        panel_sep(r++);
    }

    /* ---- Atalhos ---- */
    panel_header(r++, "CONTROLES");
    panel_text(r++, CLR_DIM, "WASD/setas mover");
    panel_text(r++, CLR_DIM, "P  pausar");
    panel_text(r++, CLR_DIM, "D  debug grafo");
    panel_text(r++, CLR_DIM, "Q  sair");

    panel_bottom(r);
}

void view_draw_hud(GameModel *m) {
    view_draw_header(m);
    view_draw_side_panel(m);
}

/* ======================== MENSAGENS NO MAPA ======================== */

void view_draw_ready(void) {
    int col = OFFSET_X + (MAP_W * CELL_W / 2) - 4;
    int row = OFFSET_Y + 19;

    move_cursor(col, row);
    printf("%sREADY!%s", CLR_READY, CLR_RESET);
}

void view_clear_ready(void) {
    int col = OFFSET_X + (MAP_W * CELL_W / 2) - 6;
    int row = OFFSET_Y + 17;
    for (int dy = 0; dy < 3; dy++) {
        move_cursor(col, row + dy);
        printf("              ");
    }
}

/* ======================== ANIMAÇÃO DE MORTE ======================== */

void view_draw_death_anim(GameModel *m, int frame) {
    const char *frames[] = {
        "ᗧ ", "◔ ", "◑ ", "◕ ", "● ", "◉ ", "○ ", "· ", "  "
    };
    int idx = frame / 2;
    if (idx >= 9) idx = 8;
    const char *colors[] = {
        CLR_PACMAN, CLR_PACMAN, CLR_PACMAN_SHADOW, CLR_ORANGE,
        CLR_ORANGE, CLR_GAMEOVER, CLR_GAMEOVER, CLR_DIM, CLR_DIM
    };
    move_cursor(OFFSET_X + m->pacman.x * CELL_W, OFFSET_Y + m->pacman.y);
    printf("%s%s%s", colors[idx], frames[idx], CLR_RESET);
}

/* ======================== BONUS FLOATS ======================== */

void view_draw_bonus(GameModel *m) {
    if (m->bonus_timer <= 0 || m->bonus_points <= 0) return;
    int sx = OFFSET_X + m->bonus_x * CELL_W;
    int sy = OFFSET_Y + m->bonus_y - (6 - m->bonus_timer) / 2;
    if (sy < OFFSET_Y) sy = OFFSET_Y;
    move_cursor(sx, sy);
    const char *col = (m->bonus_points >= 800)  ? CLR_TITLE :
                      (m->bonus_points >= 400)  ? CLR_ORANGE :
                      (m->bonus_points >= 200)  ? CLR_CYAN :
                                                  CLR_GREEN;
    printf("%s+%d%s", col, m->bonus_points, CLR_RESET);
}

void view_draw_game(GameModel *m) {
    reset_cursor();
    view_draw_map(m);
    view_draw_hud(m);

    if (m->ready_timer > 0)
        view_draw_ready();

    view_draw_entities(m);
    view_flush();
}

/* ======================== CAIXAS DECORATIVAS ======================== */

static void draw_box_double(int col, int row, int w, int h, const char *color) {
    move_cursor(col, row);
    printf("%s╔", color);
    for (int i = 0; i < w - 2; i++) printf("═");
    printf("╗%s", CLR_RESET);

    for (int r = row + 1; r < row + h - 1; r++) {
        move_cursor(col, r);             printf("%s║%s", color, CLR_RESET);
        move_cursor(col + w - 1, r);     printf("%s║%s", color, CLR_RESET);
    }

    move_cursor(col, row + h - 1);
    printf("%s╚", color);
    for (int i = 0; i < w - 2; i++) printf("═");
    printf("╝%s", CLR_RESET);
}

static void draw_box_round(int col, int row, int w, int h, const char *color) {
    move_cursor(col, row);
    printf("%s╭", color);
    for (int i = 0; i < w - 2; i++) printf("─");
    printf("╮%s", CLR_RESET);

    for (int r = row + 1; r < row + h - 1; r++) {
        move_cursor(col, r);             printf("%s│%s", color, CLR_RESET);
        move_cursor(col + w - 1, r);     printf("%s│%s", color, CLR_RESET);
    }

    move_cursor(col, row + h - 1);
    printf("%s╰", color);
    for (int i = 0; i < w - 2; i++) printf("─");
    printf("╯%s", CLR_RESET);
}

/* Caixa com sombra (efeito 3D) — usada em menu/splash/scores/help */
static void draw_box_shadow(int col, int row, int w, int h) {
    /* Sombra abaixo/direita */
    for (int r = row + 1; r < row + h; r++) {
        move_cursor(col + w, r);
        printf("%s ▓%s", CLR_DIM2, CLR_RESET);
    }
    move_cursor(col + 1, row + h);
    printf("%s", CLR_DIM2);
    for (int i = 0; i < w + 1; i++) printf("▓");
    printf("%s", CLR_RESET);

    draw_box_double(col, row, w, h, CLR_WALL_HI);
    if (w >= 6 && h >= 6)
        draw_box_round(col + 1, row + 1, w - 2, h - 2, CLR_PANEL_BORDER);
}

static void draw_divider(int col, int row, int w) {
    move_cursor(col, row);
    printf("%s╠", CLR_WALL_HI);
    for (int i = 0; i < w - 2; i++) printf("═");
    printf("╣%s", CLR_RESET);
}

/* ======================== SPLASH ARCADE ======================== */
/*
 * Tela inicial estilo arcade clássico: logo gigante, tabela de
 * pontos, atract-mode (Pac-Man fugindo dos fantasmas) e
 * "PRESSIONE ENTER" piscando. Qualquer tecla avança ao menu.
 */
void view_draw_splash(GameModel *m) {
    static int drawn = 0;
    
    if (!drawn) {
        view_clear();
        drawn = 1;
        reset_cursor();
    }

    int W = 74, H = 28;
    int bx = 4, by = 1;
    
    if (!drawn) {
        draw_box_shadow(bx, by, W, H);

        /* Logo centralizado */
        int cy = by + 3;
        int logo_x = bx + (W - 60) / 2;
        print_abs(logo_x, cy,   CLR_TITLE,     "██████╗  █████╗  ██████╗      ███╗   ███╗ █████╗ ███╗   ██╗");
        print_abs(logo_x, cy+1, CLR_TITLE,     "██╔══██╗██╔══██╗██╔════╝      ████╗ ████║██╔══██╗████╗  ██║");
        print_abs(logo_x, cy+2, CLR_TITLE_ALT, "██████╔╝███████║██║           ██╔████╔██║███████║██╔██╗ ██║");
        print_abs(logo_x, cy+3, CLR_TITLE_ALT, "██╔═══╝ ██╔══██║██║           ██║╚██╔╝██║██╔══██║██║╚██╗██║");
        print_abs(logo_x, cy+4, CLR_TITLE,     "██║     ██║  ██║╚██████╗      ██║ ╚═╝ ██║██║  ██║██║ ╚████║");
        print_abs(logo_x, cy+5, CLR_TITLE,     "╚═╝     ╚═╝  ╚═╝ ╚═════╝      ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝");

        /* Subtítulo */
        {
            const char *sub1 = "CONSOLE  EDITION   ·   ARCADE  CLASSIC";
            const char *sub2 = "UniCesumar  ·  Estruturas de Dados";
            print_abs(bx + (W - 38) / 2, cy + 7, CLR_SUBTITLE, sub1);
            print_abs(bx + (W - 34) / 2, cy + 8, CLR_DIM,      sub2);
        }

        /* Tabela de pontuação */
        int sy = by + 15;
        print_abs(bx + (W - 46) / 2, sy, CLR_DIM,
                  "─────────────  PONTUAÇÃO  ─────────────");

        int px = bx + 8;
        print_abs(px,      sy+1, CLR_PELLET, "·");
        print_abs(px + 3,  sy+1, CLR_DIM,    "10 PTS");
        print_abs(px,      sy+2, CLR_POWER,  "●");
        print_abs(px + 3,  sy+2, CLR_DIM,    "50 PTS");

        print_abs(px + 18, sy+1, CLR_BLINKY, "ᗣ");
        print_abs(px + 21, sy+1, CLR_DIM,    "200 PTS");
        print_abs(px + 18, sy+2, CLR_PINKY,  "ᗣ");
        print_abs(px + 21, sy+2, CLR_DIM,    "400 PTS");

        print_abs(px + 38, sy+1, CLR_INKY,   "ᗣ");
        print_abs(px + 41, sy+1, CLR_DIM,    "800 PTS");
        print_abs(px + 38, sy+2, CLR_CLYDE,  "ᗣ");
        print_abs(px + 41, sy+2, CLR_DIM,    "1600 PTS");
    }

    /* Atract mode: Pac-Man perseguido por fantasmas */
    int aw = W - 12;
    int total = aw + 12;
    int t = m->frame_count % total;
    int ay = by + H - 5;

    move_cursor(bx + 4, ay);
    for (int i = 0; i < W - 8; i++) printf(" ");

    int pac_x  = bx + 4 + t;
    int blinky = bx + 4 + t - 3;
    int pinky  = bx + 4 + t - 5;
    int inky   = bx + 4 + t - 7;
    int clyde  = bx + 4 + t - 9;

    int pellet_x = bx + 4 + ((t + aw/2) % total);
    if (pellet_x < bx + W - 4 && pellet_x > bx + 4) {
        move_cursor(pellet_x, ay);
        printf("%s ●%s", CLR_POWER, CLR_RESET);
    }

    if (clyde  > bx + 4 && clyde  < bx + W - 4) { move_cursor(clyde,  ay); printf("%sᗣ %s", CLR_CLYDE,  CLR_RESET); }
    if (inky   > bx + 4 && inky   < bx + W - 4) { move_cursor(inky,   ay); printf("%sᗣ %s", CLR_INKY,   CLR_RESET); }
    if (pinky  > bx + 4 && pinky  < bx + W - 4) { move_cursor(pinky,  ay); printf("%sᗣ %s", CLR_PINKY,  CLR_RESET); }
    if (blinky > bx + 4 && blinky < bx + W - 4) { move_cursor(blinky, ay); printf("%sᗣ %s", CLR_BLINKY, CLR_RESET); }
    if (pac_x  > bx + 4 && pac_x  < bx + W - 4) {
        const char *sp = ((m->frame_count / 3) % 2) ? "ᗧ " : "● ";
        move_cursor(pac_x, ay); printf("%s%s%s", CLR_PACMAN, sp, CLR_RESET);
    }

    /* "PRESSIONE ENTER" piscando */
    int blink = (m->frame_count / 10) % 2;
    int press_y = by + H - 3;
    move_cursor(bx + 4, press_y);
    for (int i = 0; i < W - 8; i++) printf(" ");
    if (blink) {
        const char *msg = "▶  PRESSIONE  ENTER  PARA  COMECAR  ◀";
        move_cursor(bx + (W - 37) / 2, press_y);
        printf("%s%s%s", CLR_TITLE, msg, CLR_RESET);
    }

    view_flush();
    m->frame_count++;
}

/* ======================== MENU PRINCIPAL (com seletor de mapas) ======================== */

static void menu_item_styled(int x, int y, int selected, const char *text, const char *hotkey, int width) {
    move_cursor(x, y);
    if (selected) {
        printf("%s%s ▶ %s%-*s   %s%s%s",
               CLR_SEL_BG, CLR_TITLE,
               CLR_TITLE, width, text,
               CLR_DIM, hotkey ? hotkey : "",
               CLR_RESET);
    } else {
        printf("   %s%-*s%s   %s%s%s",
               CLR_SUBTITLE, width, text, CLR_RESET,
               CLR_DIM2, hotkey ? hotkey : "",
               CLR_RESET);
    }
}

void view_draw_menu(GameModel *m) {
    static int drawn = 0;
    static int last_sel  = -1;
    static int last_map  = -1;
    static int last_blink = -1;
    static int last_anim  = -1;

    int bx = 3, by = 1;
    int bw = 78, bh = 38;

    if (!drawn) {
        view_clear();
        draw_box_shadow(bx, by, bw, bh);
    }

    /* ── Logo Hogwarts / Harry Potter gigante ── */
    int cy = by + 2;
    int logo_x = bx + 5;
    if (!drawn) {
        print_abs(logo_x, cy,   CLR_HOGWARTS,      "██╗  ██╗ ██████╗  ██████╗ ██╗    ██╗ █████╗ ██████╗ ████████╗███████╗");
        print_abs(logo_x, cy+1, CLR_HOGWARTS_GLOW, "██║  ██║██╔═══██╗██╔════╝ ██║    ██║██╔══██╗██╔══██╗╚══██╔══╝██╔════╝");
        print_abs(logo_x, cy+2, CLR_HOGWARTS,      "███████║██║   ██║██║  ███╗██║ █╗ ██║███████║██████╔╝   ██║   ███████╗");
        print_abs(logo_x, cy+3, CLR_HOGWARTS_GLOW, "██╔══██║██║   ██║██║   ██║██║███╗██║██╔══██║██╔══██╗   ██║   ╚════██║");
        print_abs(logo_x, cy+4, CLR_HOGWARTS,      "██║  ██║╚██████╔╝╚██████╔╝╚███╔███╔╝██║  ██║██║  ██║   ██║   ███████║");
        print_abs(logo_x, cy+5, CLR_HOGWARTS_GLOW, "╚═╝  ╚═╝ ╚═════╝  ╚═════╝  ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚══════╝");
    }

    /* Sub título + tema */
    cy += 7;
    if (!drawn) {
        const char *sub = "Harry Potter e o Labirinto de Hogwarts";
        int slen = (int)strlen(sub);
        print_abs(bx + (bw - slen) / 2, cy, CLR_TITLE_ALT, sub);

        const char *sub2 = "Escola de Magia e Bruxaria de Hogwarts";
int slen2 = (int)strlen(sub2);
print_abs(bx + (bw - slen2) / 2, cy + 1, CLR_DIM, sub2);

const char *sub3 = "\"Draco Dormiens Nunquam Titillandus\"";
int slen3 = (int)strlen(sub3);
print_abs(bx + (bw - slen3) / 2, cy + 2, CLR_SUBTITLE, sub3);

const char *sub4 = "⚜ BST  ⚜ AVL  ⚜ Grafos  ⚜ Sorts ⚜";
int slen4 = (int)strlen(sub4) - 6;
print_abs(bx + (bw - slen4) / 2 + 2, cy + 3, CLR_TITLE_ALT, sub4);
    }

    /* Atract animado (Harry + dementadores) */
    cy += 6;
    {
        int ax = bx + 6;
        int ay = cy;
        int w = bw - 12;
        int total = w + 10;
        int t = (m->frame_count / 2) % total;

        if (t != last_anim || !drawn) {
            last_anim = t;
            move_cursor(ax, ay);
            for (int i = 0; i < w; i++) printf(" ");

            int pellet_x = ax + ((t + w/2) % total);
            if (pellet_x < ax + w - 1 && pellet_x > ax) {
                move_cursor(pellet_x, ay);
                printf("%s ●%s", CLR_POWER, CLR_RESET);
            }

            int pac  = ax + t;
            int b    = ax + t - 3;
            int p    = ax + t - 5;
            int in   = ax + t - 7;
            int c    = ax + t - 9;

            if (c  > ax && c  < ax + w - 1) { move_cursor(c,  ay); printf("%sᗣ %s", CLR_CLYDE,  CLR_RESET); }
            if (in > ax && in < ax + w - 1) { move_cursor(in, ay); printf("%sᗣ %s", CLR_INKY,   CLR_RESET); }
            if (p  > ax && p  < ax + w - 1) { move_cursor(p,  ay); printf("%sᗣ %s", CLR_PINKY,  CLR_RESET); }
            if (b  > ax && b  < ax + w - 1) { move_cursor(b,  ay); printf("%sᗣ %s", CLR_BLINKY, CLR_RESET); }
            if (pac> ax && pac< ax + w - 1) {
                const char *sp = ((m->frame_count / 3) % 2) ? "ᗧ " : "● ";
                move_cursor(pac, ay); printf("%s%s%s", CLR_PACMAN, sp, CLR_RESET);
            }
        }
    }

    /* Divisor */
    cy += 2;
    if (!drawn) draw_divider(bx, cy, bw);

    /* ── SELETOR DE MAPA + PREVIEW (lado a lado) ── */
    cy += 2;
    const char *maps[MAP_COUNT] = {
    "Classico",
    "Arena aberta",
    "Labirinto duro",
    "Corredores Hogwarts",
    "Camara Secreta"
};

const char *descs[MAP_COUNT] = {
    "Equilibrado, fiel ao original.",
    "Mais espaco aberto pra correr.",
    "Mais paredes, curvas e risco.",
    "Corredores longos do castelo.",
    "A Camara Secreta esta aberta."
};
    /* Mini-previews ASCII (4 linhas x ~17 chars) */
    const char *preview[MAP_COUNT][4] = {
    {
        "#######  ########",
        "#.....#  #.....#",
        "#.###.####.###.#",
        "#######  ########"
    },
    {
        "#...............#",
        "#  o  . .  o    #",
        "#... + + + +  ..#",
        "#...............#"
    },
    {
        "#####  ##  ## ###",
        "#...#  ..  #..  #",
        "###.#  ##  #.####",
        "#...#  ##     ..#"
    },
    {
        "#..#..#..#..#..#",
        "#..#..#..#..#..#",
        "#..#..#..#..#..#",
        "#..#..#..#..#..#"
    },
    {
        "################",
        "#....##....##..#",
        "#.##.##.##.##..#",
        "################"
    }
};

    if (!drawn) print_abs(bx + 4, cy, CLR_GREEN, "▼ MAPA");
    if (!drawn) print_abs(bx + bw/2 + 4, cy, CLR_GREEN, "▼ PREVIEW");

    /* Esquerda: seleção do mapa, sempre redesenha quando muda */
    if (!drawn || m->map_selected != last_map) {
        last_map = m->map_selected;
        clear_rect(bx + 4, cy + 1, bw/2 - 8, 6);
        move_cursor(bx + 4, cy + 1);
        printf("%s◀%s  %s%-14s%s  %s▶%s",
               CLR_CYAN, CLR_RESET,
               CLR_TITLE, maps[m->map_selected], CLR_RESET,
               CLR_CYAN, CLR_RESET);
        move_cursor(bx + 4, cy + 3);
        printf("%s%s%s", CLR_SUBTITLE, descs[m->map_selected], CLR_RESET);
        move_cursor(bx + 4, cy + 5);
        printf("%s← → ou M para trocar%s", CLR_DIM, CLR_RESET);

        /* Preview na metade direita: 4 linhas de 16 chars usando # e . */
        clear_rect(bx + bw/2 + 4, cy + 1, 20, 5);
        for (int i = 0; i < 4; i++) {
            move_cursor(bx + bw/2 + 4, cy + 1 + i);
            const char *line = preview[m->map_selected][i];
            for (const char *p = line; *p; p++) {
                char c = *p;
                if (c == '#') {
                    printf("%s█%s", CLR_WALL, CLR_RESET);
                } else if (c == '.') {
                    printf("%s·%s", CLR_PELLET, CLR_RESET);
                } else if (c == 'o' || c == '+' || c == '*') {
                    printf("%s●%s", CLR_POWER, CLR_RESET);
                } else if (c == '-') {
                    printf("%s═%s", CLR_DOOR, CLR_RESET);
                } else {
                    putchar(' ');
                }
            }
        }
    }

    /* Divisor antes do menu */
    cy += 7;
    if (!drawn) draw_divider(bx, cy, bw);

    /* ── Menu de opções ── */
    cy += 2;
    if (!drawn || m->menu_selected != last_sel) {
        last_sel = m->menu_selected;
        int mx = bx + 18;
        menu_item_styled(mx, cy + 0, m->menu_selected == 0, "Jogar (Harry)",          "[ENTER]", 30);
        menu_item_styled(mx, cy + 1, m->menu_selected == 1, "Modo Feitico Auto","[A]    ", 30);
        menu_item_styled(mx, cy + 2, m->menu_selected == 2, "Aula de Magia",          "[E]    ", 30);
        menu_item_styled(mx, cy + 3, m->menu_selected == 3, "Copa das Casas",              "[H]    ", 30);
        menu_item_styled(mx, cy + 4, m->menu_selected == 4, "Sair",                     "[Q]    ", 30);
    }

    cy += 7;
    {
        int blink = (m->frame_count / 12) % 2;
        if (!drawn || blink != last_blink) {
            last_blink = blink;
            move_cursor(bx + 4, cy);
            for (int i = 0; i < bw - 8; i++) printf(" ");
            if (blink == 0) {
                const char *msg = "▲ ▼  navegar  ·  ENTER  selecionar  ·  ◀ ▶  trocar mapa";
                print_abs(bx + (bw - 56) / 2, cy, CLR_SUBTITLE, msg);
            }
        }
    }

    /* Rodapé arcade */
    {
        const char *foot = " HOGWARTS EXPRESS  ·  PLATAFORMA 9 3/4  ·  © 2026 ";
        int flen = 38;
        move_cursor(bx + (bw - flen) / 2, by + bh - 3);
        printf("%s%s%s", CLR_DIM, foot, CLR_RESET);
    }

    view_flush();
    m->frame_count++;
    drawn = 1;
}

/* ======================== GAME OVER ======================== */

void view_draw_gameover(GameModel *m) {
    view_clear();
    
    int W = 72, H = 16;
    int cx = 2;
    int cy = 2;

    draw_box_shadow(cx, cy, W, H);

    int tx = cx + 3;
    int ty = cy + 2;
print_abs(cx + 8, cy + 3, CLR_GREEN, "███╗   ███╗ ██████╗ ██████╗ ██████╗ ███████╗██╗   ██╗");
print_abs(cx + 8, cy + 4, CLR_GREEN, "████╗ ████║██╔═══██╗██╔══██╗██╔══██╗██╔════╝██║   ██║");
print_abs(cx + 8, cy + 5, CLR_GREEN, "██╔████╔██║██║   ██║██████╔╝██████╔╝█████╗  ██║   ██║");
print_abs(cx + 8, cy + 6, CLR_GREEN, "██║╚██╔╝██║██║   ██║██╔══██╗██╔══██╗██╔══╝  ██║   ██║");
print_abs(cx + 8, cy + 7, CLR_GREEN, "██║ ╚═╝ ██║╚██████╔╝██║  ██║██║  ██║███████╗╚██████╔╝");
print_abs(cx + 8, cy + 8, CLR_GREEN, "╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝ ╚═════╝ ");
                                        print_abs(cx + 18, cy + 9, CLR_SUBTITLE,
                                            "O Lorde das Trevas venceu...");

    move_cursor(cx + 4, cy + H - 4);
    printf("%sSCORE FINAL%s    %s%6d%s", CLR_DIM, CLR_RESET, CLR_SCORE, m->score, CLR_RESET);
    move_cursor(cx + 28, cy + H - 4);
    printf("%sHI-SCORE%s    %s%6d%s", CLR_DIM, CLR_RESET, CLR_TITLE, m->high_score, CLR_RESET);

    move_cursor(cx + 4, cy + H - 3);
    printf("%sENTER%s menu  ·  %sH%s scores  ·  %sQ%s sair",
           CLR_CYAN, CLR_DIM, CLR_CYAN, CLR_DIM, CLR_CYAN, CLR_DIM);

    view_flush();
}

/* ======================== VITORIA ======================== */

void view_draw_win(GameModel *m) {
    /* Flash dramático nas paredes */
    const char *flash_colors[] = {
        CLR_TITLE, CLR_TITLE_ALT, CLR_GREEN, CLR_CYAN, CLR_MAGENTA, CLR_TITLE
    };
    for (int f = 0; f < 12; f++) {
        const char *clr = flash_colors[f % 6];
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++)
                if (m->grid[y][x] == TILE_WALL) {
                    const char *g = wall_glyph(m, x, y);
                    move_cursor(OFFSET_X + x * CELL_W, OFFSET_Y + y);
                    printf("%s%s%s", clr, g, CLR_RESET);
                }
        view_flush();
        sleep_ms(120);
    }

    int W = 56, H = 18;
    int cx = OFFSET_X + (MAP_W * CELL_W - W) / 2;
    int cy = OFFSET_Y + (MAP_H - H) / 2;
    draw_box_shadow(cx, cy, W, H);

    print_abs(cx + 12, cy + 2, CLR_WIN_CLR, "★  L A B I R I N T O   C O M P L E T O  ★");

    move_cursor(cx + 4, cy + 4);
    printf("%sSCORE FINAL%s   %s%d%s    %sNIVEL%s   %s%d%s",
           CLR_DIM, CLR_RESET, CLR_TITLE, m->score, CLR_RESET,
           CLR_DIM, CLR_RESET, CLR_CYAN, m->level, CLR_RESET);

    /* Benchmark de sorting */
    move_cursor(cx + 4, cy + 6);
    printf("%sBENCHMARK DE ORDENAÇÃO  (μs)%s", CLR_GREEN, CLR_RESET);

    const char *sort_names[7] = {"Bubble","Selection","Insertion","Shell","Merge","Quick","Heap"};

    long long max_t = 1;
    for (int i = 0; i < 7; i++)
        if (m->sort_times[i] > max_t) max_t = m->sort_times[i];

    for (int i = 0; i < 7; i++) {
        int rr = cy + 7 + i;
        move_cursor(cx + 4, rr);
        printf("%s%-10s%s ", CLR_CYAN, sort_names[i], CLR_RESET);

        int bw = 26;
        int filled = (int)((m->sort_times[i] * bw) / max_t);
        if (filled < 1 && m->sort_times[i] > 0) filled = 1;
        if (filled > bw) filled = bw;
        printf("%s", CLR_TITLE_ALT);
        for (int j = 0; j < filled; j++)       printf("█");
        printf("%s", CLR_DIM2);
        for (int j = filled; j < bw; j++)      printf("░");
        printf("%s  %s%6lld%s μs",
               CLR_RESET, CLR_SCORE, m->sort_times[i], CLR_RESET);
    }

    move_cursor(cx + 4, cy + H - 2);
    printf("%sPressione ENTER para continuar%s", CLR_DIM, CLR_RESET);

    view_flush();
}

/* ======================== HIGH SCORES (com pódio) ======================== */

void view_draw_scores(GameModel *m) {
    view_clear();

    int W = 64, H = 34;
    int cx = 6, cy = 1;
    draw_box_shadow(cx, cy, W, H);

    print_abs(cx + 16, cy + 2, CLR_TITLE,
              "★  T A C A   D A S   C A S A S  ★");
    print_abs(cx + 18, cy + 3, CLR_DIM,
              "BST + 7 Sorts + Pergaminho ASCII");

    int scores[20];
    char names[20][16];
    int count = 0;
    bst_inorder_rev(m->score_tree, scores, names, &count, 10);

    int sorted[20];
    memcpy(sorted, scores, count * sizeof(int));
    if (count > 1) quick_sort(sorted, 0, count - 1);

    /* ── Pódio para top 3 ── */
    int podium_y = cy + 5;
    int col2 = cx + 8;
    int col1 = cx + 24;
    int col3 = cx + 44;

    if (count >= 2) {
        char buf[48];
        snprintf(buf, sizeof(buf), " %-7s ", names[1]);
        print_abs(col2, podium_y,     CLR_SUBTITLE, buf);
        snprintf(buf, sizeof(buf), " %5d ", scores[1]);
        print_abs(col2, podium_y + 1, CLR_CYAN, buf);
    }
    if (count >= 1) {
        char buf[48];
        snprintf(buf, sizeof(buf), "  ★ %-5s ★", names[0]);
        print_abs(col1, podium_y - 1, CLR_TITLE, buf);
        snprintf(buf, sizeof(buf), "    %6d  ", scores[0]);
        print_abs(col1, podium_y,     CLR_TITLE, buf);
    }
    if (count >= 3) {
        char buf[48];
        snprintf(buf, sizeof(buf), " %-7s ", names[2]);
        print_abs(col3, podium_y + 2, CLR_SUBTITLE, buf);
        snprintf(buf, sizeof(buf), " %5d ", scores[2]);
        print_abs(col3, podium_y + 3, CLR_ORANGE, buf);
    }

    /* Blocos do pódio */
    if (count >= 2) {
        print_abs(col2, podium_y + 2, CLR_CYAN, " ┌─────┐ ");
        print_abs(col2, podium_y + 3, CLR_CYAN, " │  2  │ ");
        print_abs(col2, podium_y + 4, CLR_CYAN, " │     │ ");
        print_abs(col2, podium_y + 5, CLR_CYAN, " └─────┘ ");
    }
    if (count >= 1) {
        print_abs(col1, podium_y + 1, CLR_TITLE, "  ┌─────┐  ");
        print_abs(col1, podium_y + 2, CLR_TITLE, "  │  1  │  ");
        print_abs(col1, podium_y + 3, CLR_TITLE, "  │     │  ");
        print_abs(col1, podium_y + 4, CLR_TITLE, "  │     │  ");
        print_abs(col1, podium_y + 5, CLR_TITLE, "  └─────┘  ");
    }
    if (count >= 3) {
        print_abs(col3, podium_y + 4, CLR_ORANGE, " ┌─────┐ ");
        print_abs(col3, podium_y + 5, CLR_ORANGE, " └─────┘ ");
    }

    /* ── Tabela 4-10 ── */
    int ty = cy + 15;
    move_cursor(cx + 4, ty);
    printf("%s  POS   NOME           PONTOS      GRADE%s", CLR_DIM, CLR_RESET);
    move_cursor(cx + 4, ty + 1);
    printf("%s──────────────────────────────────────────%s", CLR_DIM2, CLR_RESET);

    const char *grades[] = {"S+","S","A+","A","B+","B","C+","C","D","F"};
    const char *gclr[] = {
        CLR_TITLE, CLR_TITLE, CLR_GREEN, CLR_GREEN,
        CLR_CYAN, CLR_CYAN, CLR_MAGENTA, CLR_MAGENTA,
        CLR_ORANGE, CLR_GAMEOVER
    };

    for (int i = 3; i < 10; i++) {
        move_cursor(cx + 4, ty + 2 + (i - 3));
        if (i < count) {
            printf("  %s%2d.%s   %s%-10s%s   %s%7d%s pts    %s%2s%s",
                   CLR_DIM, i + 1, CLR_RESET,
                   CLR_SUBTITLE, names[i], CLR_RESET,
                   CLR_SCORE, scores[i], CLR_RESET,
                   gclr[i], grades[i], CLR_RESET);
        } else {
            printf("  %s%2d.   ---           -------         -%s",
                   CLR_DIM2, i + 1, CLR_RESET);
        }
    }

    /* Pergaminho ASCII da BST */
    char tree_lines[8][96];
    int tree_n = 0;
    bst_print_ascii(m->score_tree, tree_lines, &tree_n, 8, 0);
    move_cursor(cx + 4, cy + H - 11);
    printf("%sPERGAMINHO BST ASCII%s", CLR_MAGENTA, CLR_RESET);
    for (int i = 0; i < tree_n && i < 5; i++) {
        move_cursor(cx + 4, cy + H - 10 + i);
        printf("%s%-52.52s%s", CLR_DIM, tree_lines[i], CLR_RESET);
    }

    /* Info estruturas */
    move_cursor(cx + 4, cy + H - 5);
    printf("%sBST%s h=%-2d n=%-2d   %sAVL%s h=%-2d b=%-2d   %sSort%s 7 algoritmos",
           CLR_CYAN, CLR_RESET,
           bst_height(m->score_tree), bst_count(m->score_tree),
           CLR_CYAN, CLR_RESET,
           avl_height(m->powerup_tree), avl_balance_factor(m->powerup_tree),
           CLR_CYAN, CLR_RESET);

    move_cursor(cx + 4, cy + H - 3);
    printf("%sENTER para voltar ao menu%s", CLR_DIM, CLR_RESET);

    view_flush();
}

/* ======================== MODO EXPLICAÇÃO ======================== */

void view_draw_help(GameModel *m) {
    (void)m;
    view_clear();

    int W = 68, H = 34;
    int bx = 4, by = 2;
    draw_box_shadow(bx, by, W, H);

    print_abs(bx + 22, by + 2, CLR_TITLE, "★  M O D O   E X P L I C A C A O  ★");
    print_abs(bx + 18, by + 3, CLR_DIM,   "Como ganhar + estruturas por trás do jogo");

    int r = by + 5;

    move_cursor(bx + 3, r++); printf("%s● OBJETIVO%s", CLR_GREEN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sColete todos os pontos do labirinto sem ser pego.%s", CLR_SUBTITLE, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sZerou os pontos → próximo nível.%s", CLR_SUBTITLE, CLR_RESET);
    r++;

    move_cursor(bx + 3, r++); printf("%s● POWER PELLET  %s ●  %s(50 pts)%s",
                                     CLR_GREEN, CLR_POWER, CLR_DIM, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sFantasmas viram azuis e podem ser comidos por:%s", CLR_SUBTITLE, CLR_RESET);
    move_cursor(bx + 5, r++); printf("  %s200 → 400 → 800 → 1600 pts%s (combo dobra a cada um)",
                                     CLR_ORANGE, CLR_RESET);
    r++;

    move_cursor(bx + 3, r++); printf("%s● FANTASMAS E SUAS IA%s", CLR_GREEN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sᗣ%s %sBlinky%s vermelho: %sDijkstra%s (caminho mínimo)",
                                     CLR_BLINKY, CLR_RESET, CLR_BLINKY, CLR_RESET, CLR_CYAN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sᗣ%s %sPinky%s  rosa:     %sBFS%s (largura, antecipa Pac-Man)",
                                     CLR_PINKY, CLR_RESET, CLR_PINKY, CLR_RESET, CLR_CYAN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sᗣ%s %sInky%s   ciano:    %sDFS%s (profundidade, imprevisível)",
                                     CLR_INKY, CLR_RESET, CLR_INKY, CLR_RESET, CLR_CYAN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sᗣ%s %sClyde%s  laranja:  %sManhattan%s (distância simples)",
                                     CLR_CLYDE, CLR_RESET, CLR_CLYDE, CLR_RESET, CLR_CYAN, CLR_RESET);
    r++;

    move_cursor(bx + 3, r++); printf("%s● CONTROLES%s", CLR_GREEN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sW A S D%s ou %s← ↑ ↓ →%s   mover Pac-Man",
                                     CLR_CYAN, CLR_RESET, CLR_CYAN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sP%s pausa    %sQ%s sair    %sH%s scores",
                                     CLR_CYAN, CLR_RESET, CLR_CYAN, CLR_RESET, CLR_CYAN, CLR_RESET);
    r++;

    move_cursor(bx + 3, r++); printf("%s● AUTO-PLAY%s", CLR_GREEN, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sSimulated Annealing planeja vários passos à frente.%s",
                                     CLR_SUBTITLE, CLR_RESET);
    move_cursor(bx + 5, r++); printf("%sNo power, muda para HUNT e prioriza fantasmas vulneráveis.%s",
                                     CLR_SUBTITLE, CLR_RESET);

    move_cursor(bx + 3, by + H - 3);
    printf("%sENTER / Esc — voltar ao menu%s", CLR_DIM, CLR_RESET);

    view_flush();
}
