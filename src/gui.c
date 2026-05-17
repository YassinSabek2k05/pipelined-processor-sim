#include "raylib.h"
#include "../include/gui.h"
#include "../include/pipeline.h"
#include "../include/memory.h"
#include "../include/loader.h"
#include "../include/structs.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ── Window / layout ───────────────────────────────────────────────────────────
#define WIN_W          1400
#define WIN_H           900
#define HEADER_H         40
#define REG_PANEL_W     200
#define TIMING_H        280
#define BOTTOM_Y        (HEADER_H + TIMING_H)   // 320
#define BOTTOM_H        (WIN_H - BOTTOM_Y)       // 580
#define MEM_PANEL_W     ((WIN_W - REG_PANEL_W) / 2)  // 600
#define ITEMS_PER_PAGE   22

// ── Timing diagram geometry ───────────────────────────────────────────────────
#define TD_LABEL_W  148
#define TD_COL_W     45
#define TD_ROW_H     22
#define TD_TITLE_H   22
#define TD_HDR_H     22

// ── Colours ───────────────────────────────────────────────────────────────────
static const Color C_BG        = {18,  18,  28,  255};
static const Color C_PANEL     = {28,  28,  44,  255};
static const Color C_BORDER    = {55,  55,  78,  255};
static const Color C_HEADER    = {14,  14,  22,  255};
static const Color C_TEXT      = {218, 218, 238, 255};
static const Color C_DIM       = {95,  95,  118, 255};
static const Color C_HILITE    = {255, 215, 50,  255};
static const Color C_GREEN     = {90,  210, 100, 255};
static const Color C_ORANGE    = {220, 140, 50,  255};

static const Color STAGE_COL[5] = {
    {55,  120, 200, 255},  // IF  – blue
    {55,  170, 75,  255},  // ID  – green
    {205, 170, 38,  255},  // EX  – yellow
    {205, 108, 38,  255},  // MEM – orange
    {148, 65,  195, 255},  // WB  – purple
};
static const char *STAGE_LBL[5] = {"IF","ID","EX","MM","WB"};

// ── Register flash tracking ───────────────────────────────────────────────────
#define FLASH_FRAMES 45
static int32_t prev_R[33];     // index 32 = PC
static int     flash_timer[33];

static void flash_update(void) {
    uint32_t cur_pc = cpu.pc;
    if ((int32_t)cur_pc != prev_R[32]) {
        prev_R[32] = (int32_t)cur_pc;
        flash_timer[32] = FLASH_FRAMES;
    } else if (flash_timer[32] > 0) flash_timer[32]--;

    for (int i = 0; i < 32; i++) {
        if (cpu.R[i] != prev_R[i]) {
            prev_R[i] = cpu.R[i];
            flash_timer[i] = FLASH_FRAMES;
        } else if (flash_timer[i] > 0) flash_timer[i]--;
    }
}

// ── Mnemonic decoder ─────────────────────────────────────────────────────────
static int32_t se18(uint32_t v) {
    return (v & 0x20000u) ? (int32_t)(v | 0xFFFC0000u) : (int32_t)v;
}

static void mnemonic(uint32_t instr, char *buf, size_t sz) {
    uint32_t op    = instr >> 28;
    uint32_t r1    = (instr >> 23) & 0x1F;
    uint32_t r2    = (instr >> 18) & 0x1F;
    uint32_t r3    = (instr >> 13) & 0x1F;
    uint32_t shamt = instr & 0x1FFF;
    uint32_t addr  = instr & 0x0FFFFFFF;
    int32_t  imm   = se18(instr & 0x3FFFF);
    switch (op) {
        case  0: snprintf(buf, sz, "ADD R%u R%u R%u",    r1, r2, r3);    break;
        case  1: snprintf(buf, sz, "SUB R%u R%u R%u",    r1, r2, r3);    break;
        case  2: snprintf(buf, sz, "MUL R%u R%u R%u",    r1, r2, r3);    break;
        case  3: snprintf(buf, sz, "MOVI R%u %d",         r1, imm);       break;
        case  4: snprintf(buf, sz, "JEQ R%u R%u %d",     r1, r2, imm);   break;
        case  5: snprintf(buf, sz, "AND R%u R%u R%u",    r1, r2, r3);    break;
        case  6: snprintf(buf, sz, "ORI R%u R%u %d",     r1, r2, imm);   break;
        case  7: snprintf(buf, sz, "JMP %u",              addr);           break;
        case  8: snprintf(buf, sz, "LSL R%u R%u %u",     r1, r2, shamt); break;
        case  9: snprintf(buf, sz, "LSR R%u R%u %u",     r1, r2, shamt); break;
        case 10: snprintf(buf, sz, "MOVR R%u R%u %d",    r1, r2, imm);   break;
        case 11: snprintf(buf, sz, "MOVM R%u R%u %d",    r1, r2, imm);   break;
        default: snprintf(buf, sz, "???");                                  break;
    }
}

// ── Helper: draw a titled panel background ────────────────────────────────────
static void panel(int x, int y, int w, int h, const char *title) {
    DrawRectangle(x, y, w, h, C_PANEL);
    DrawRectangleLines(x, y, w, h, C_BORDER);
    if (title) {
        DrawRectangle(x + 1, y + 1, w - 2, TD_TITLE_H - 1, C_HEADER);
        DrawText(title, x + 6, y + 4, 13, C_DIM);
    }
}

// ── Header bar ────────────────────────────────────────────────────────────────
static void draw_header(bool auto_run, bool done) {
    DrawRectangle(0, 0, WIN_W, HEADER_H, C_HEADER);
    DrawRectangleLines(0, 0, WIN_W, HEADER_H, C_BORDER);

    DrawText("Pipelined Processor Simulator", 10, 11, 16, C_TEXT);

    char cyc[32];
    snprintf(cyc, sizeof(cyc), "Cycle: %u", cpu.cycles);
    DrawText(cyc, 310, 11, 16, C_HILITE);

    const char *status = done ? "DONE" : (auto_run ? "RUNNING" : "PAUSED");
    Color sc = done ? C_GREEN : (auto_run ? C_ORANGE : C_DIM);
    DrawText(status, 430, 11, 15, sc);

    DrawText("SPACE=Step  ENTER=Run/Pause  R=Reset  H/J=InstrMem  K/L=DataMem",
             540, 12, 13, C_DIM);
}

// ── Register panel ────────────────────────────────────────────────────────────
static void draw_registers(void) {
    int x = 0, y = HEADER_H, w = REG_PANEL_W, h = WIN_H - HEADER_H;
    panel(x, y, w, h, "REGISTERS");

    int items   = 33; // PC + R0-R31
    int avail_h = h - TD_TITLE_H - 4;
    int item_h  = avail_h / items;
    if (item_h < 14) item_h = 14;
    int fs = (item_h >= 18) ? 15 : 13;

    int fy = y + TD_TITLE_H + 2;

    // PC
    char buf[32];
    Color col = (flash_timer[32] > 0) ? C_HILITE : C_TEXT;
    snprintf(buf, sizeof(buf), "PC: %u", cpu.pc);
    DrawText(buf, x + 6, fy + (item_h - fs) / 2, fs, col);
    fy += item_h;
    DrawLine(x + 4, fy, x + w - 4, fy, C_BORDER);

    // R0–R31
    for (int i = 0; i < 32; i++) {
        col = (flash_timer[i] > 0) ? C_HILITE : C_TEXT;
        snprintf(buf, sizeof(buf), "R%-2d: %d", i, cpu.R[i]);
        DrawText(buf, x + 6, fy + (item_h - fs) / 2, fs, col);
        fy += item_h;
    }
}

// ── Pipeline timing diagram ───────────────────────────────────────────────────
static void draw_timing(void) {
    int x = REG_PANEL_W;
    int y = HEADER_H;
    int w = WIN_W - REG_PANEL_W;
    int h = TIMING_H;

    panel(x, y, w, h, "PIPELINE TIMING   [IF] [ID] [EX] [MM] [WB]");

    // Draw legend inline in title bar
    int lx = x + w - 300;
    int ly = y + 4;
    for (int s = 0; s < 5; s++) {
        DrawRectangle(lx, ly + 1, 18, 14, STAGE_COL[s]);
        DrawText(STAGE_LBL[s], lx + 20, ly + 2, 12, C_TEXT);
        lx += 55;
    }

    // Collect unique PCs (sorted by address)
    int pcs[MAX_GUI_HISTORY];
    int pc_count = 0;
    for (int c = 0; c < gui_snapshot_count; c++) {
        for (int s = 0; s < 5; s++) {
            int p = gui_snapshots[c].stage_pc[s];
            if (p < 0) continue;
            bool found = false;
            for (int i = 0; i < pc_count; i++) {
                if (pcs[i] == p) { found = true; break; }
            }
            if (!found && pc_count < MAX_GUI_HISTORY) pcs[pc_count++] = p;
        }
    }
    for (int i = 0; i < pc_count - 1; i++)
        for (int j = i + 1; j < pc_count; j++)
            if (pcs[j] < pcs[i]) { int t = pcs[i]; pcs[i] = pcs[j]; pcs[j] = t; }

    int data_area_w  = w - TD_LABEL_W;
    int max_cols     = data_area_w / TD_COL_W;
    int data_area_h  = h - TD_TITLE_H - TD_HDR_H;
    int max_rows     = data_area_h / TD_ROW_H;

    // Auto-scroll so newest cycle is always visible
    int col_offset = 0;
    if (gui_snapshot_count > max_cols) col_offset = gui_snapshot_count - max_cols;

    int hdr_y  = y + TD_TITLE_H;
    int data_y = hdr_y + TD_HDR_H;

    // Cycle-number header row
    DrawRectangle(x, hdr_y, TD_LABEL_W, TD_HDR_H, C_HEADER);
    DrawText("Instruction", x + 4, hdr_y + 4, 11, C_DIM);

    int cols_to_draw = (gui_snapshot_count - col_offset);
    if (cols_to_draw > max_cols) cols_to_draw = max_cols;

    for (int ci = 0; ci < cols_to_draw; ci++) {
        int cx = x + TD_LABEL_W + ci * TD_COL_W;
        DrawRectangle(cx, hdr_y, TD_COL_W, TD_HDR_H, C_HEADER);
        DrawRectangleLines(cx, hdr_y, TD_COL_W, TD_HDR_H, C_BORDER);
        char nbuf[16];
        snprintf(nbuf, sizeof(nbuf), "%d", col_offset + ci + 1);
        int tw = MeasureText(nbuf, 12);
        DrawText(nbuf, cx + (TD_COL_W - tw) / 2, hdr_y + 5, 12, C_DIM);
    }

    // Instruction rows
    int rows_to_draw = (pc_count < max_rows) ? pc_count : max_rows;
    for (int ri = 0; ri < rows_to_draw; ri++) {
        int pc  = pcs[ri];
        int ry  = data_y + ri * TD_ROW_H;

        // Label column
        char lbl[36];
        char mn[24] = "";
        uint32_t instr = read_memory((uint32_t)pc);
        mnemonic(instr, mn, sizeof(mn));
        snprintf(lbl, sizeof(lbl), "%d: %s", pc, mn);
        DrawRectangle(x, ry, TD_LABEL_W, TD_ROW_H, C_PANEL);
        DrawRectangleLines(x, ry, TD_LABEL_W, TD_ROW_H, C_BORDER);
        DrawText(lbl, x + 4, ry + 4, 11, C_TEXT);

        // Stage cells
        for (int ci = 0; ci < cols_to_draw; ci++) {
            int cycle_i = col_offset + ci;
            int cx      = x + TD_LABEL_W + ci * TD_COL_W;

            DrawRectangle(cx, ry, TD_COL_W, TD_ROW_H, C_PANEL);
            DrawRectangleLines(cx, ry, TD_COL_W, TD_ROW_H, C_BORDER);

            for (int s = 0; s < 5; s++) {
                if (gui_snapshots[cycle_i].stage_pc[s] == pc) {
                    DrawRectangle(cx + 1, ry + 1, TD_COL_W - 2, TD_ROW_H - 2, STAGE_COL[s]);
                    int tw2 = MeasureText(STAGE_LBL[s], 12);
                    DrawText(STAGE_LBL[s], cx + (TD_COL_W - tw2) / 2, ry + 5, 12, WHITE);
                    break;
                }
            }
        }
    }
}

// ── Memory panel (shared for instr and data) ──────────────────────────────────
static void draw_memory(int x, int y, int w, int h, bool is_instr, int *page) {
    const char *title = is_instr
        ? "INSTRUCTION MEMORY  [H=Prev  J=Next]"
        : "DATA MEMORY  [K=Prev  L=Next]";
    panel(x, y, w, h, title);

    int base   = is_instr ? 0 : DATA_SEGMENT_START;
    int seg_sz = is_instr ? DATA_SEGMENT_START : (MEMORY_SIZE - DATA_SEGMENT_START);
    int total_pages = (seg_sz + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    if (*page < 0)            *page = 0;
    if (*page >= total_pages) *page = total_pages - 1;

    // Page indicator
    char pbuf[32];
    snprintf(pbuf, sizeof(pbuf), "Page %d / %d", *page + 1, total_pages);
    int ptw = MeasureText(pbuf, 13);
    DrawText(pbuf, x + (w - ptw) / 2, y + 24, 13, C_DIM);

    // Column headers
    int fy = y + TD_TITLE_H + 22;
    DrawText("Addr",  x + 6,   fy, 12, C_DIM);
    DrawText("Hex",   x + 52,  fy, 12, C_DIM);
    if (is_instr) DrawText("Instruction", x + 148, fy, 12, C_DIM);
    else          DrawText("Value",       x + 148, fy, 12, C_DIM);
    fy += 15;
    DrawLine(x + 2, fy, x + w - 2, fy, C_BORDER);
    fy += 3;

    int item_h   = 19;
    int start    = base + (*page) * ITEMS_PER_PAGE;
    int end      = start + ITEMS_PER_PAGE;
    if (end > base + seg_sz) end = base + seg_sz;

    for (int addr = start; addr < end; addr++) {
        uint32_t val = read_memory((uint32_t)addr);

        // Highlight: current PC (next fetch) or instruction inside pipeline
        bool is_pc  = is_instr && (uint32_t)addr == cpu.pc;
        bool in_pip = is_instr && (
            (cpu.if_id.valid  && cpu.if_id.pc  == (uint32_t)addr) ||
            (cpu.id_ex.valid  && cpu.id_ex.pc  == (uint32_t)addr) ||
            (cpu.ex_mem.valid && cpu.ex_mem.pc == (uint32_t)addr) ||
            (cpu.mem_wb.valid && cpu.mem_wb.pc == (uint32_t)addr));

        Color row_bg  = is_pc  ? (Color){50, 48, 15, 255} :
                        in_pip ? (Color){18, 38, 55, 255} : C_PANEL;
        Color txt_col = is_pc  ? C_HILITE :
                        in_pip ? (Color){130, 190, 255, 255} : C_TEXT;
        Color adr_col = is_pc  ? C_HILITE :
                        in_pip ? (Color){100, 160, 220, 255} : C_DIM;

        DrawRectangle(x + 1, fy, w - 2, item_h - 1, row_bg);

        char addr_s[16], hex_s[12], val_s[32];
        snprintf(addr_s, sizeof(addr_s), "%04d", addr);
        snprintf(hex_s,  sizeof(hex_s),  "0x%08X", val);

        DrawText(addr_s, x + 6,  fy + 2, 12, adr_col);
        DrawText(hex_s,  x + 48, fy + 2, 12, txt_col);

        if (is_instr) {
            if (val != 0) {
                mnemonic(val, val_s, sizeof(val_s));
                DrawText(val_s, x + 148, fy + 2, 12, txt_col);
            }
        } else {
            if (val != 0) {
                snprintf(val_s, sizeof(val_s), "%d", (int32_t)val);
                DrawText(val_s, x + 148, fy + 2, 12, C_GREEN);
            }
        }

        DrawLine(x + 2, fy + item_h - 1, x + w - 2, fy + item_h - 1, C_BORDER);
        fy += item_h;
    }
}

// ── Reset helper ──────────────────────────────────────────────────────────────
static void do_reset(const char *filename) {
    init_memory();
    load_program(filename);
    init_processor();
    gui_snapshot_count = 0;
    memset(prev_R,      0, sizeof(prev_R));
    memset(flash_timer, 0, sizeof(flash_timer));
}

// ── Main GUI entry point ──────────────────────────────────────────────────────
void run_gui(const char *filename) {
    do_reset(filename);

    InitWindow(WIN_W, WIN_H, "Pipelined Processor Simulator");
    SetTargetFPS(60);

    bool  auto_run     = false;
    double run_interval = 0.20;   // seconds per cycle when auto-running (~5 Hz)
    double last_step   = 0.0;

    int instr_page = 0;
    int data_page  = 0;

    while (!WindowShouldClose()) {

        // ── Input ─────────────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_SPACE) && cpu.running) {
            step_cycle();
            flash_update();
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (cpu.running) auto_run = !auto_run;
        }
        if (IsKeyPressed(KEY_R)) {
            do_reset(filename);
            auto_run  = false;
            instr_page = 0;
            data_page  = 0;
        }
        if (IsKeyPressed(KEY_H)) { if (instr_page > 0) instr_page--; }
        if (IsKeyPressed(KEY_J)) instr_page++;
        if (IsKeyPressed(KEY_K)) { if (data_page > 0) data_page--; }
        if (IsKeyPressed(KEY_L)) data_page++;

        if (!cpu.running) auto_run = false;

        if (auto_run) {
            double now = GetTime();
            if (now - last_step >= run_interval) {
                step_cycle();
                flash_update();
                last_step = now;
            }
        }

        // Auto-follow PC in instruction memory while running
        if (cpu.running) {
            int target_page = (int)cpu.pc / ITEMS_PER_PAGE;
            if (!IsKeyDown(KEY_H) && !IsKeyDown(KEY_J))
                instr_page = target_page;
        }

        // ── Draw ──────────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground(C_BG);

        draw_header(auto_run, !cpu.running);
        draw_registers();
        draw_timing();

        draw_memory(REG_PANEL_W,               BOTTOM_Y, MEM_PANEL_W, BOTTOM_H, true,  &instr_page);
        draw_memory(REG_PANEL_W + MEM_PANEL_W, BOTTOM_Y, MEM_PANEL_W, BOTTOM_H, false, &data_page);

        // Vertical divider between memory panels
        DrawLine(REG_PANEL_W + MEM_PANEL_W, BOTTOM_Y,
                 REG_PANEL_W + MEM_PANEL_W, WIN_H, C_BORDER);

        EndDrawing();
    }

    CloseWindow();
}