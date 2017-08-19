#include "buttons_through_shift_register_one_in.h"
#include <string.h>

buttons_through_shift_register_one_in::buttons_through_shift_register_one_in ( buttons_through_shift_register_one_in_cfg* cfg ) : cfg( cfg ) {}

void buttons_through_shift_register_one_in::init() {
    USER_OS_STATIC_TASK_CREATE( this->task, "b_sr", BUTTONS_THROUGH_SHIFT_REGISTER_ONE_IN_TASK_STACK_SIZE, ( void* )this, this->cfg->prio, this->task_stack, &this->task_struct );
    uint32_t b_count = this->cfg->pin_count;
    // Обязательно очищаем поля статуса всех клавиш.
    for ( uint32_t l = 0; l < b_count; l++ ) {
        this->cfg->p_pin_conf_array[l].status->press                    = false;
        this->cfg->p_pin_conf_array[l].status->bounce                   = false;
        this->cfg->p_pin_conf_array[l].status->event_long_click         = false;
        this->cfg->p_pin_conf_array[l].status->bounce_time              = 0;
        this->cfg->p_pin_conf_array[l].status->button_long_click_time   = 0;
    }
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
    sr_one_in_button_status_struct* s       = this->cfg->p_pin_conf_array[ b_number ].status; // Для удобства записи.
    sr_one_in_button_item_cfg*      p_st    = &this->cfg->p_pin_conf_array[ b_number ];

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
                // Информируем пользователя, если он этого желает (указал семафоры и очереди).
                if ( p_st->s_press != nullptr )
                    USER_OS_GIVE_BIN_SEMAPHORE( *p_st->s_press );
                if ( p_st->q_press != nullptr )
                    USER_OS_QUEUE_SEND( *p_st->q_press, &p_st->v_press, 0 );                    // Кладем в очеред без ожидания.
            }
        } else {                        // Если проверка на дребезг уже прошла, при этом клавишу еще держат.
            if ( p_st->dl_delay_ms == 0 ) return;                                               // Если мы не отслеживаем длительное нажатие - выходим.
            if ( s->event_long_click == true ) return;                                          // Если мы уже отдали флаг о том, что нажатие длительное - выходим.
            s->button_long_click_time += this->cfg->delay_ms;                                   // Прибавляем время, прошедшее с последней проверки.
            if ( s->button_long_click_time < p_st->dl_delay_ms ) return;                        // Если прошло времени меньше, чем нужно для того, чтобы кнопка считалась долго зажатой - выходим.
            s->event_long_click = true;                                                         // Дождались, кнопка долго зажата.
            // Информируем пользователя, если он этого желает (указал семафоры и очереди).
            if ( p_st->s_start_long_press != nullptr )
                USER_OS_GIVE_BIN_SEMAPHORE( *p_st->s_start_long_press );
            if ( p_st->q_start_long_press != nullptr )
                USER_OS_QUEUE_SEND( *p_st->q_start_long_press, &p_st->v_start_long_press, 0 );
        }
    }
}

// Обработать отпущенную клавишу.
void buttons_through_shift_register_one_in::process_not_press ( const uint32_t &b_number ) {
    sr_one_in_button_status_struct* s       = this->cfg->p_pin_conf_array[ b_number ].status;
    sr_one_in_button_item_cfg*      p_st    = &this->cfg->p_pin_conf_array[ b_number ];

    if ( s->press == false ) return;                                                            // Если она и до этого была отпущена - выходим.
    s->press = false;

    if ( s->bounce == true ) {                                                                  // Если кнопка проходила проверку на дребезг, но не прошла.
        s->bounce                   = false;
        return;
    }

    // Если проверку состояния мы все-таки прошли.
    if ( s->event_long_click == true ) {                                                        // Если произошло длительное нажатие.
        s->event_long_click         = false;
        // Информируем пользователя, если он этого желает (указал семафоры и очереди).
        if ( p_st->s_release_long_click != nullptr )
            USER_OS_GIVE_BIN_SEMAPHORE( *p_st->s_release_long_click );
        if ( p_st->q_release_long_click != nullptr )
            USER_OS_QUEUE_SEND( *p_st->q_release_long_click, &p_st->v_release_long_click, 0 );
        return;
    }

    // Если это было короткое нажатие.
    if ( p_st->s_release_click != nullptr )
        USER_OS_GIVE_BIN_SEMAPHORE( *p_st->s_release_click );
    if ( p_st->q_release_click != nullptr )
        USER_OS_QUEUE_SEND( *p_st->q_release_click, &p_st->v_release_click, 0 );
}

void buttons_through_shift_register_one_in::task ( void* p_obj ) {
    buttons_through_shift_register_one_in* o = ( buttons_through_shift_register_one_in* )p_obj;
    while ( true ) {
        if ( o->cfg->mutex != nullptr )
            USER_OS_TAKE_MUTEX( *o->cfg->mutex, portMAX_DELAY );    // sdcard занята нами.

        for ( uint32_t b_l = 0; b_l < o->cfg->pin_count; b_l++ ) {
            o->select_button( b_l );
            if ( o->check_press() == true ) {
                o->process_press( b_l );
            } else {
                o->process_not_press( b_l );
            }
        }

        if ( o->cfg->mutex != nullptr )
            USER_OS_GIVE_MUTEX( *o->cfg->mutex );

        USER_OS_DELAY_MS( o->cfg->delay_ms );
    }
}
