#include "buttons_through_shift_register_one_in.h"
#include <string.h>

buttons_through_shift_register_one_in::buttons_through_shift_register_one_in ( buttons_through_shift_register_one_in_cfg* cfg ) : cfg( cfg ) {
    USER_OS_STATIC_TASK_CREATE( this->task, "b_sr", BUTTONS_THROUGH_SHIFT_REGISTER_ONE_IN_TASK_STACK_SIZE, ( void* )this, this->cfg->prio, this->task_stack, &this->task_struct );
}

// Выбрать нужную кнопку через сдвиговом регистре (подать на ее вывод 0, а на остальные 1).
void buttons_through_shift_register_one_in::select_button ( const uint32_t &b_number ) {        // Значение b_number можно не проверять, т.к. это внутренний метод и на уровне выше уже проверено.
    memset( this->cfg->p_button_array, 0, this->cfg->sr_buffer_byte_count );                    // Не опрашиваемые в данный момент кнопки == 0.
    this->cfg->p_button_array[ this->cfg->p_pin_conf_array[ b_number ].byte ] |=
        1 << this->cfg->p_pin_conf_array[ b_number ].bit;                                       // Ставим 1 на ножке (т.к. вывод подтянут к 0, то при нажатой клавише мы сразу полочим 1 на входе).
    this->cfg->p_sr->write();                                                                   // Перезаписываем все в сдвиговом регистре.
}

// Метод возвращает true - если клавиша нажата, false, если нет.
bool buttons_through_shift_register_one_in::check_press ( void ) {
    return this->cfg->p_in_pin->read();
}

// Обработать нажатую клавишу.
void buttons_through_shift_register_one_in::process_press ( const uint32_t &b_number ) {
    // Для удобства записи.
    sr_one_in_button_status_struct* s       = this->cfg->p_pin_conf_array->status;
    sr_one_in_button_item_cfg*      p_st    = this->cfg->p_pin_conf_array;

    if ( s->press == false ) {  // Если до этого момента кнопка была сброшена.
        s->press        = true;                                                                 // Показываем, что нажатие произошло.
        s->bounce       = true;                                                                 // Начинаем проверку на дребезг.
        s->bounce_time  = p_st->ms_stabil;                                                      // Устанавливаем время для проверки дребезга контактов.
    } else {                    // Если мы уже некоторое время держим эту кнопку.
        if ( s->bounce == true ) {      // Если идет проверка на дребезг.
            if ( s->bounce_time - this->cfg->delay_ms > 0 ) {   // Если время проверки еще не закончилось (мы от оставшегося времени отнимаем время, прошедшее с предыдущей проверки).
                s->bounce_time -= this->cfg->delay_ms;
            } else {                                            // Если время проверки закончилось и, при этом, все это время была зажата, то успех.
                s->bounce           = false;                                                    // Проверка больше не проводится.
                s->bounce_time      = 0;                                                        // Сбросим и время.
                s->event_bounce     = true;                                                     // Говорим, что проверка была успешно пройдена.

                // Информируем пользователя, если он этого желает (указал семафоры и очереди).
                if ( p_st->s_press != nullptr )
                    USER_OS_GIVE_BIN_SEMAPHORE( *p_st->s_press );
                if ( p_st->q_press != nullptr )
                    USER_OS_QUEUE_SEND( *q_press, &v_press, 0 );                                // Кладем в очеред без ожидания.
            }
        } else {                        // Если проверка на дребезг уже прошла, при этом клавишу еще держат.
        }
    }
}
   else {
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



// Обработать отпущенную клавишу.
void buttons_through_shift_register_one_in::process_not_press ( const uint32_t &b_number ) {

}

void buttons_through_shift_register_one_in::task ( void* p_obj ) {
    buttons_through_shift_register_one_in* o = ( buttons_through_shift_register_one_in* )p_obj;
    while ( true ) {
        for ( uint32_t b_l = 0; b_l < o->cfg->pin_count; b_l++ ) {
            o->select_button( b_l );
            if ( this->check_press() == true ) {
                this->process_press( b_l );
            } else {

            }
        }
        USER_OS_DELAY_MS( o->cfg->delay_ms );
    }
}



// Если клавишу отпустили (или была отпущена).
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



*/
