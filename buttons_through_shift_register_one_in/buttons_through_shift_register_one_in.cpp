#include "buttons_through_shift_register_one_in.h"

buttons_through_shift_register_one_in::buttons_through_shift_register_one_in ( buttons_through_shift_register_one_in_cfg* cfg ) : cfg( cfg ) {

}

// Маски бит в общем флаге.
// Структура button_state для 1-й кнопки:
// 0-й бит: Состояние кнопки в данный момент: 1 нажата, 0 - сброшена.
// 1-й бит: Событие "прошла проверку дребезга (была нажата)": 1 - нажали, 0 - нет.
// 2-й бит: Событие "была отпущена". 1 - была отпущена. 0 - нет.
// 3-й бит: Событие "произошло длительное нажатие": 1 - произошло, 0 - нет.
// 4-й бит: В данный момент идет проверка на дребезг: 1 - идет, 0 - нет.
// 8-15: оставшееся время до окончания проверки на дребезг (в мс). А после - сколько клавиша ужа нажата: 8-24.

/*
#define BUTTON_STATE_MSK        (1<<0)        // Маска флага состояния вывода в данный момент.
#define BUTTON_BOUNCE_MSK        (1<<4)        // Маска Флага проверки на дребезг: 1 - идет, 0 - нет.
#define BUTTON_EVENT_BOUNCE     (1<<1)        // Маска события "прошла проверку дребезга (была нажата)": 1 - нажали, 0 - нет.
#define BUTTOM_EVENT_LONG_CLICK (1<<3)         // Маска события "произошло длительное нажатие": 1 - произошло, 0 - нет.
#define BUTTON_BOUNCE_TIME_MSK     0xFF00        // Маска оставшегося времени до окончания проверки на дребезг (в мс).
#define BUTTON_LONG_CLICK_TIME_MSK     0xFFFFFF00// Маска временисколько, сколько клавиша ужа нажата.
*/

/*
void button_to_shift_register_task (button_to_shift_register_t *button){
    while(1){
        for (uint32_t button_loop = 0; button_loop<button->cfg->count_pin; button_loop++){    // Смотрим состояние всех подключенных кнопок.
            button->cfg->point_button_array[button->cfg->pin_conf[button_loop].byte] &= ~(1<<button->cfg->pin_conf[button_loop].bit);    // Ставим 0 на ножке (т.к. вывод подтянут к 1, то при нажатой клавише мы сразу полочим 0 на входе).
            shift_reg_write(*button->cfg->fd_shift_register, button->cfg->point_array_reg);    // Перезаписываем все в сдвиговом регистре.
            if (port_gpio_read(button->cfg->in_pin) == 0){    // Если 0, то кнопка зажата.
                if ((button->button_state[button_loop] & BUTTON_STATE_MSK) == 0){    // Если до этого момента кнопка была сброшена.
                    button->button_state[button_loop] |= BUTTON_STATE_MSK|    // Показываем, что нажатие произошло.
                            (1<<4)|    // Начинаем проверку на дребезг.
                            (button->cfg->pin_conf[button_loop].ms_stabil<<8);    // Выставляем время для тестирования дребезга.
                } else {    // Если мы уже некоторое время держим эту кнопку.
                    if ((button->button_state[button_loop] & BUTTON_BOUNCE_MSK) != 0) {    // Если идет проверка на дребезг.
                        if ((((uint8_t)(button->button_state[button_loop] >> 8)) - button->cfg->delay) > 0){    // Если время проверки еще не закончилось (мы от оставшегося времени отнимаем время, прошедшее с предыдущей проверки).
                            uint8_t buffer = (uint8_t)(button->button_state[button_loop] >> 8);    // Сохраняем оставшееся время.
                            button->button_state[button_loop] &= ~BUTTON_BOUNCE_TIME_MSK;        // Чистим его место.
                            button->button_state[button_loop] |= (buffer - button->cfg->delay)<<8;    // Сохраняем оставшееся время - сколько прошло.
                        } else { // Если время проверки закончилось и, при этом, все это время была зажата, то успех.
                            button->button_state[button_loop] &= ~((uint16_t)(BUTTON_BOUNCE_MSK|(BUTTON_BOUNCE_TIME_MSK)));    // Проверка на дребизг завершена (за одно и сбрасываем оставшееся время, оно может быть чуть меньше, чем прошло с момента последней проверки).
                            button->button_state[button_loop] |= BUTTON_EVENT_BOUNCE;    // Кнопка прошла проверку дребезга и теперь зажата.
                            if (button->cfg->pin_conf[button_loop].semaphore_click != NULL){    // Если по этому событию (прошла проверку дребезга) требуется отдать флаг (он указан), отдаем.
                                xSemaphoreGive(*button->cfg->pin_conf[button_loop].semaphore_click);    // Отдаем симафор.
                            };
                            if (button->cfg->pin_conf[button_loop].queue_click != NULL){    // Если есть очередь, которая ждет данных.
                                xQueueSend( *button->cfg->pin_conf[button_loop].queue_click, &button->cfg->pin_conf[button_loop].value_click, portMAX_DELAY );
                            }
                        };
                    } else { // Если проверка на дребезг уже прошла, при этом клавишу еще держат.
                        if (button->cfg->pin_conf[button_loop].flag_dl == 1){    // Если мы отслеживаем длительное нажатие.
                            if (((uint16_t)(button->button_state[button_loop]>>8) + button->cfg->delay >= button->cfg->pin_conf[button_loop].dl_delay_ms) && // Если прошло уже больше времени, чем требуется для "долгого нажатия"
                                    ((button->button_state[button_loop] & BUTTOM_EVENT_LONG_CLICK) == 0)){// И при этом флаг раньше выставлен не был.
                                button->button_state[button_loop] |= BUTTOM_EVENT_LONG_CLICK;    // Выставляем флаг о том, что уже дождались.
                                if (button->cfg->pin_conf[button_loop].semaphore_start_long_press != NULL){    // Если по этому событию (началось длительное нажатие) требуется отдать флаг (он указан), отдаем.
                                    xSemaphoreGive(*button->cfg->pin_conf[button_loop].semaphore_start_long_press);    // Отдаем симафор.
                                };
                                if (button->cfg->pin_conf[button_loop].queue_start_long_press != NULL){    // Если есть очередь, которая ждет данных.
                                    xQueueSend( *button->cfg->pin_conf[button_loop].queue_start_long_press, &button->cfg->pin_conf[button_loop].value_start_long_press, portMAX_DELAY );
                                }
                            } else {
                                uint16_t buffer = (uint16_t)(button->button_state[button_loop]>>8);    // Сохранем прошедшее время.
                                button->button_state[button_loop] &= ~BUTTON_LONG_CLICK_TIME_MSK;    // Сбрасываем то, что было (освобождаем место.
                                button->button_state[button_loop] |= (buffer + button->cfg->delay)<<8;// Добавляем к тому, что было, время с последней проверки.
                            }
                        }
                    }
                };
            } else {    // Если клавишу отпустили (или была отпущена).
                if ((button->button_state[button_loop] & BUTTON_STATE_MSK) != 0){    // Если кнопка до этого не была отпущена.
                    if ((button->button_state[button_loop] & BUTTON_BOUNCE_MSK) != 0){    // Если кнопка проходила проверку на дребезг, но не прошла.
                        button->button_state[button_loop] = 0;    // Сбрасываем все состояния. Кнопка проверку не прошла.
                    } else {    // Если проверку состояния мы все-таки прошли.
                        if ((button->button_state[button_loop] & BUTTOM_EVENT_LONG_CLICK) != 0){        // Если произошло длительное нажатие.
                            if (button->cfg->pin_conf[button_loop].semaphore_release_long_click != NULL){    // Если по этому событию (длительное нажатие) требуется отдать флаг (он указан), отдаем.
                                xSemaphoreGive(*button->cfg->pin_conf[button_loop].semaphore_release_long_click);    // Отдаем симафор.
                            };
                            if (button->cfg->pin_conf[button_loop].queue_release_release_long_click != NULL){    // Если есть очередь, которая ждет данных.
                                xQueueSend( *button->cfg->pin_conf[button_loop].queue_release_release_long_click, &button->cfg->pin_conf[button_loop].value_release_release_long_click, portMAX_DELAY );
                            }
                            button->button_state[button_loop] = 0;
                        } else {// Если было, все таки, короткое нажатие.
                            if (button->cfg->pin_conf[button_loop].semaphore_release_click != NULL){    // Если по этому событию (короткое) требуется отдать флаг (он указан), отдаем.
                                xSemaphoreGive(*button->cfg->pin_conf[button_loop].semaphore_release_click);    // Отдаем симафор.
                            };
                            if (button->cfg->pin_conf[button_loop].queue_release_click != NULL){    // Если есть очередь, которая ждет данных.
                                xQueueSend( *button->cfg->pin_conf[button_loop].queue_release_click, &button->cfg->pin_conf[button_loop].value_release_click, portMAX_DELAY );
                            }
                            button->button_state[button_loop] = 0;
                        }
                    }
                };
            };
            button->cfg->point_button_array[button->cfg->pin_conf[button_loop].byte] |= 1<<button->cfg->pin_conf[button_loop].bit;    // Ставим 1 на ножке (возвращаем в исходное положение).
        };
        vTaskDelay(button->cfg->delay);            // После просмотра состояний всех клавиш - ждем.
    };
};



*/
