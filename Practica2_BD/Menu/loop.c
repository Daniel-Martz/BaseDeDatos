#include "loop.h"
#include "odbc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sql.h>
#include <sqlext.h>
#include <ncurses.h>

#define N_ROWS_PAGE 8

/**
 * @brief Bucle principal de eventos de la aplicación.
 * @author Rodrigo Diaz-Reganon
 * 
 * @param windows Puntero a la estructura _Windows que contiene todas las ventanas.
 * @param menus   Puntero a la estructura _Menus. 
 * @param forms   Puntero a la estructura _Forms.
 * @param panels  Puntero a la estructura _Panels 
 */
void loop(_Windows *windows, _Menus *menus, _Forms *forms, _Panels *panels)
{
    int focus = FOCUS_LEFT;
    int ch = 0;
    bool enterKey = FALSE;
    char buffer[1024];
    ITEM *auxItem = NULL;
    int choice = -1;
    MENU *menu = menus->menu;
    WINDOW *menu_win = windows->menu_win;
    WINDOW *out_win = windows->out_win;
    WINDOW *msg_win = windows->msg_win;
    int out_highlight = 0;
    int n_out_choices = 0;
    int current_page = 0;
    int total_pages = 0;
    int out_win_capacity = windows->rows_out_win;
    int i = 0;
    int start;
    int end;
    char *tmpStr1 = NULL;
    int initial_capacity;
    int line_length;
    char *linea;

    char salida_str[30];
    char llegada_str[30];
    int num_vuelos;
    int num_plazas;
    long flight_id1;
    long flight_id2;
    char ac1_str[10];
    char ac2_str[10];

    keypad(stdscr, TRUE);
    keypad(menu_win, TRUE);
    keypad(out_win, TRUE);
    curs_set(1);

    while (TRUE)
    {
        ch = getch();
        auxItem = current_item(menu);

        switch (ch)
        {
        case KEY_LEFT:
        case 0x3C:
            focus = FOCUS_LEFT;
            menu_driver(menu, REQ_LEFT_ITEM);
            wrefresh(menu_win);
            auxItem = current_item(menu);
            if (item_index(auxItem) == SEARCH)
                top_panel(panels->search_panel);
            else if (item_index(auxItem) == BPASS)
                top_panel(panels->bpass_panel);
            update_panels();
            doupdate();
            break;

        case KEY_RIGHT:
        case 0x3E:
            focus = FOCUS_LEFT;
            menu_driver(menu, REQ_RIGHT_ITEM);
            wrefresh(menu_win);
            auxItem = current_item(menu);
            if (item_index(auxItem) == SEARCH)
                top_panel(panels->search_panel);
            else if (item_index(auxItem) == BPASS)
                top_panel(panels->bpass_panel);
            update_panels();
            doupdate();
            break;

        case KEY_UP:
        case 0x2B:
            if (item_index(auxItem) == SEARCH && focus == FOCUS_LEFT)
            {
                form_driver(forms->search_form, REQ_PREV_FIELD);
                form_driver(forms->search_form, REQ_END_LINE);
                wrefresh(windows->form_search_win);
            }
            else if (item_index(auxItem) == BPASS && focus == FOCUS_LEFT)
            {
                form_driver(forms->bpass_form, REQ_PREV_FIELD);
                form_driver(forms->bpass_form, REQ_END_LINE);
                wrefresh(windows->form_bpass_win);
            }

            else if (focus == FOCUS_RIGHT && n_out_choices > 0)
            {
                out_highlight = MAX(out_highlight - 1, 0);

                /* Si el cursor se sale por arriba de la página, retrocede página */
                start = current_page * N_ROWS_PAGE;
                if (out_highlight < start && current_page > 0)
                {
                    current_page--;
                }
                start = current_page * N_ROWS_PAGE;
                end = MIN(start + N_ROWS_PAGE, n_out_choices);
                wclear(out_win);
                print_out(out_win, &menus->out_win_choices[start], end - start, out_highlight - start, windows->out_title);
                snprintf(buffer, sizeof(buffer), "Página %d/%d", current_page + 1, total_pages);
                write_msg(msg_win, buffer, -1, -1, windows->msg_title);
            }
            break;

        case KEY_DOWN:
            if (item_index(auxItem) == SEARCH && focus == FOCUS_LEFT)
            {
                form_driver(forms->search_form, REQ_NEXT_FIELD);
                form_driver(forms->search_form, REQ_END_LINE);
                wrefresh(windows->form_search_win);
            }
            else if (item_index(auxItem) == BPASS && focus == FOCUS_LEFT)
            {
                form_driver(forms->bpass_form, REQ_NEXT_FIELD);
                form_driver(forms->bpass_form, REQ_END_LINE);
                wrefresh(windows->form_bpass_win);
            }
            else if (focus == FOCUS_RIGHT && n_out_choices > 0)
            {
                out_highlight = MIN(out_highlight + 1, n_out_choices - 1);

                /* Si el cursor se sale por abajo de la página actual, avanza página */
                end = MIN((current_page * N_ROWS_PAGE) + N_ROWS_PAGE, n_out_choices);
                if (out_highlight >= end && current_page < total_pages - 1)
                {
                    current_page++;
                }

                start = current_page * N_ROWS_PAGE;
                end = MIN(start + N_ROWS_PAGE, n_out_choices);
                wclear(out_win);
                print_out(out_win, &menus->out_win_choices[start], end - start, out_highlight - start, windows->out_title);
                snprintf(buffer, sizeof(buffer), "Página %d/%d", current_page + 1, total_pages);
                write_msg(msg_win, buffer, -1, -1, windows->msg_title);
            }
            break;

        case KEY_NPAGE:
            if (focus == FOCUS_RIGHT && total_pages > 1 && current_page < total_pages - 1)
            {
                current_page++;
                start = current_page * N_ROWS_PAGE;
                end = MIN(start + N_ROWS_PAGE, n_out_choices);
                if (out_highlight < start)
                    out_highlight = start;
                else if (out_highlight >= end)
                    out_highlight = end - 1;
                wclear(out_win);
                print_out(out_win, &menus->out_win_choices[start], end - start, out_highlight - start, windows->out_title);
                snprintf(buffer, sizeof(buffer), "Página %d/%d", current_page + 1, total_pages);
                write_msg(msg_win, buffer, -1, -1, windows->msg_title);
            }
            break;

        case KEY_PPAGE:
            if (focus == FOCUS_RIGHT && total_pages > 1 && current_page > 0)
            {
                current_page--;
                start = current_page * N_ROWS_PAGE;
                end = MIN(start + N_ROWS_PAGE, n_out_choices);

                if (out_highlight < start)
                    out_highlight = start;
                else if (out_highlight >= end)
                    out_highlight = end - 1;

                wclear(out_win);
                print_out(out_win, &menus->out_win_choices[start], end - start, out_highlight - start, windows->out_title);
                snprintf(buffer, sizeof(buffer), "Página %d/%d", current_page + 1, total_pages);
                write_msg(msg_win, buffer, -1, -1, windows->msg_title);
            }
            break;

        case KEY_STAB:
        case 9:
            focus = (focus == FOCUS_RIGHT) ? FOCUS_LEFT : FOCUS_RIGHT;
            snprintf(buffer, sizeof(buffer), "focus in window %d", focus);
            write_msg(msg_win, buffer, -1, -1, windows->msg_title);
            break;

        case KEY_BACKSPACE:
        case 127:
            if (item_index(auxItem) == SEARCH)
            {
                form_driver(forms->search_form, REQ_DEL_PREV);
                wrefresh(windows->form_search_win);
            }
            else if (item_index(auxItem) == BPASS)
            {
                form_driver(forms->bpass_form, REQ_DEL_PREV);
                wrefresh(windows->form_bpass_win);
            }
            break;

        case 10: /* enter */
            choice = item_index(auxItem);
            enterKey = TRUE;
            break;

        default:
            if (item_index(auxItem) == SEARCH)
            {
                form_driver(forms->search_form, ch);
                wrefresh(windows->form_search_win);
            }
            else if (item_index(auxItem) == BPASS)
            {
                form_driver(forms->bpass_form, ch);
                wrefresh(windows->form_bpass_win);
            }
            break;
        }

        if (choice != -1 && enterKey)
        {
            /*Variables para gestión de memoria*/
            initial_capacity = windows->rows_out_win;
            line_length = windows->cols_out_win - 4;

            if (choice == QUIT)
                break;

            /* SEARCH desde form window */
            if (choice == SEARCH && focus == FOCUS_LEFT)
            {
                if (menus->out_win_choices != NULL)
                {
                    for (i = 0; i < out_win_capacity; i++)
                    {
                        if ((menus->out_win_choices)[i] != NULL)
                        {
                            free((menus->out_win_choices)[i]);
                        }
                    }
                    free(menus->out_win_choices);
                }
                menus->out_win_choices = (char **)calloc(initial_capacity, sizeof(char *));
                if (menus->out_win_choices == NULL)
                    break;
                for (i = 0; i < initial_capacity; i++)
                {
                    (menus->out_win_choices)[i] = (char *)calloc(line_length, sizeof(char));
                    if ((menus->out_win_choices)[i] == NULL)
                        break;
                }
                out_win_capacity = initial_capacity;
                n_out_choices = 0;
                out_highlight = 0;
                wclear(out_win);
                form_driver(forms->search_form, REQ_VALIDATION);
                tmpStr1 = field_buffer((forms->search_form_items)[1], 0);
                results_search(tmpStr1,
                               field_buffer((forms->search_form_items)[3], 0),
                               field_buffer((forms->search_form_items)[5], 0),
                               &n_out_choices, &(menus->out_win_choices),
                               line_length, initial_capacity);

                if (n_out_choices > 0 &&
                    (strncmp(menus->out_win_choices[0], "Error", 5) == 0))
                {
                    write_msg(msg_win, menus->out_win_choices[0], -1, -1, windows->msg_title);
                    n_out_choices = 0;
                }

                if (n_out_choices > 0)
                {
                    total_pages = (n_out_choices + N_ROWS_PAGE - 1) / N_ROWS_PAGE;
                    current_page = 0;
                    out_highlight = 0;
                    start = current_page * N_ROWS_PAGE;
                    end = MIN(start + N_ROWS_PAGE, n_out_choices);
                    wclear(out_win);
                    print_out(out_win, &menus->out_win_choices[start], end - start, out_highlight - start, windows->out_title);
                }

                if (n_out_choices > out_win_capacity)
                    out_win_capacity = n_out_choices;
            }
            /*SEARCH desde out window */
            else if (choice == SEARCH && focus == FOCUS_RIGHT)
            {
                if (n_out_choices == 0)
                {
                    choice = -1;
                    enterKey = FALSE;
                    continue;
                }

                linea = menus->out_win_choices[out_highlight];

                if (out_highlight == 0 &&
                    (strncmp(linea, "Error", 5) == 0))
                {
                    write_msg(msg_win, linea, -1, -1, windows->msg_title);
                }
                else
                {
                    /* sscanf que COINCIDE con el snprintf de search.c */
                    int n = sscanf(linea,
                                   "Salida: %29[^|] | Llegada: %29[^|] | Vuelos: %d | Plazas: %d | INFO: %ld %9s %ld %9s",
                                   salida_str,
                                   llegada_str,
                                   &num_vuelos,
                                   &num_plazas,
                                   &flight_id1,
                                   ac1_str,
                                   &flight_id2,
                                   ac2_str);

                    /* comprueba si leyó 8 elementos */
                    if (n < 8)
                    {
                        snprintf(buffer, sizeof(buffer), "Error: no se pudo leer la línea. (n=%d)", n);
                        write_msg(msg_win, buffer, -1, -1, windows->msg_title);
                    }
                    else
                    {
                        /* muestra los datos */
                        if (flight_id2 == -1)
                        {
                            /* Vuelo directo */
                            snprintf(buffer, sizeof(buffer),
                                     "DETALLES: Vuelo ID %ld (Aircraft_code %s). Salida: %s. Llegada: %s",
                                     flight_id1, ac1_str, salida_str, llegada_str);
                        }
                        else
                        {
                            /* Vuelo con escala */
                            snprintf(buffer, sizeof(buffer),
                                     "DETALLES: Vuelo1(ID %ld, Aircraft_code %s) -> Vuelo2(ID %ld, Aircraft_code %s). Salida: %s. Llegada: %s",
                                     flight_id1, ac1_str, flight_id2, ac2_str, salida_str, llegada_str);
                        }
                        write_msg(msg_win, buffer, -1, -1, windows->msg_title);
                    }
                }
            }
            /* BPASS desde form window */
            else if (choice == BPASS && focus == FOCUS_LEFT)
            {
                if (menus->out_win_choices != NULL)
                {
                    for (i = 0; i < out_win_capacity; i++)
                    {
                        if ((menus->out_win_choices)[i] != NULL)
                        {
                            free((menus->out_win_choices)[i]);
                        }
                    }
                    free(menus->out_win_choices);
                }
                menus->out_win_choices = (char **)calloc(initial_capacity, sizeof(char *));
                if (menus->out_win_choices == NULL)
                    break;
                for (i = 0; i < initial_capacity; i++)
                {
                    (menus->out_win_choices)[i] = (char *)calloc(line_length, sizeof(char));
                    if ((menus->out_win_choices)[i] == NULL)
                        break;
                }
                out_win_capacity = initial_capacity;
                n_out_choices = 0;
                out_highlight = 0;
                wclear(out_win);
                form_driver(forms->bpass_form, REQ_VALIDATION);
                tmpStr1 = field_buffer((forms->bpass_form_items)[1], 0);

                results_bpass(tmpStr1, &n_out_choices, &(menus->out_win_choices),
                              line_length, initial_capacity);

                if (n_out_choices > 0 &&
                    (strncmp(menus->out_win_choices[0], "Error", 5) == 0))
                {
                    write_msg(msg_win, menus->out_win_choices[0], -1, -1, windows->msg_title);
                    n_out_choices = 0;
                }

                if (n_out_choices > 0)
                {
                    total_pages = (n_out_choices + N_ROWS_PAGE - 1) / N_ROWS_PAGE;
                    current_page = 0;
                    out_highlight = 0;
                    start = current_page * N_ROWS_PAGE;
                    end = MIN(start + N_ROWS_PAGE, n_out_choices);
                    wclear(out_win);
                    print_out(out_win, &menus->out_win_choices[start], end - start, out_highlight - start, windows->out_title);
                }
                if (n_out_choices > out_win_capacity)
                    out_win_capacity = n_out_choices;
            }

            /* BPASS desde out window */
            else if (choice == BPASS && focus == FOCUS_RIGHT)
            {
                if (n_out_choices == 0)
                {
                    choice = -1;
                    enterKey = FALSE;
                    continue;
                }
                linea = menus->out_win_choices[out_highlight];
                if (out_highlight == 0 &&
                    (strncmp(linea, "Error", 5) == 0))
                {
                    write_msg(msg_win, linea, -1, -1, windows->msg_title);
                }
                else
                {
                    write_msg(msg_win, linea, -1, -1, windows->msg_title);
                }
            }
            choice = -1;
            enterKey = FALSE;
        }
    }
}