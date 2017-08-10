#pragma once

#include "mk_hardware_interfaces_pin.h"
#include "module_shift_register.h"
#include "user_os.h"

// Структура для каждого вывода, подключенного через сдвиговый регистр.
struct sr_one_in_button_item_cfg {
    // Положение кнопкаи в сдвиговом регистре.
    uint8_t                             byte;                           // Бит в сдвиговом регистре, на котором весит кнопка (счет с 0).
    uint8_t                             bit;                            // Байт, в котором находится кнопка (бит кнопки), счет с 0.
                                                                        // Причем 0-й байт - это point_button_array.
                                                                        // А он может быть уже 3-м или каким еще относительно point_array_reg.

    // Отслеживание дребезга контактов.
    uint8_t                             ms_stabil;                      // Колличество мс, для стабилизации нажатия (защита от дребезга).

    // Отслеживание разных видов нажатия.
    bool                                flag_dl;                        // Флаг разрешения отслеживания длительного нажатия false - длительное нажатие не отслеживается, true - отслеживается.
    uint16_t                            dl_delay_ms;                    // Время нажатия, после которого кнопка считается долго зажатой. В миллисекундах.
    USER_OS_STATIC_BIN_SEMAPHORE*       s_click;                        // Семафор, который будет отдан сразу же после стабилизации нажатия (по клику). Если указан.
    USER_OS_STATIC_BIN_SEMAPHORE*       s_release_click;                // Семафор, который будет отдан сразу же после отпуска кнопки (при коротком). Если указан.
    USER_OS_STATIC_BIN_SEMAPHORE*       s_release_long_click;           // Симафор, который будет отдан сразу же, после отпускания кнопки с длительным нажатием.
    USER_OS_STATIC_BIN_SEMAPHORE*       s_start_long_press;             // Симафор, который будет отдан сразу же, как только пройдет время, после которого кнопка будет считаться удерживаемой долго, но до ее отпускания.
    USER_OS_STATIC_QUEUE*               q_click;                        // Очередь, в которую будет положено значение сразу же после нажатия клавиши (с учетом стабилизации от дребезга).
    uint8_t                             v_click;                        // Значение, которое будет положено в queue_click.
    USER_OS_STATIC_QUEUE*               q_release_click;                // Очередь, в которую будет положено значение сразу же после отпускания кнопки (короткого).
    uint8_t                             v_release_click;
    USER_OS_STATIC_QUEUE*               q_release_release_long_click;   // Очередь, в которую будет записано значение сразу после отпускания (длительного).
    uint8_t                             v_release_release_long_click;
    USER_OS_STATIC_QUEUE*               q_start_long_press;             // Очередь, в которую будет записано значение сразу же, как только пройдет время, после которого кнопка будет считаться удерживаемой долго, но до ее отпускания.
    uint8_t                             v_start_long_press;
} ;

struct buttons_through_shift_register_one_in_cfg {
    pin_base*                   p_in_pin;               // Общий для всех кнопок вывод. Обязательно должен быть подтянут к +!
    module_shift_register*      p_sr;                   // HC595 или аналог. Шина данных идет через подключенный сдвиговый регистр.
    sr_one_in_button_item_cfg*  p_pin_conf_array;       // Указатель на массив с описанием выводов.
    uint32_t                    pin_count;              // Колличество выводов в массиве.
    uint32_t                    delay;                  // Пауза между опросами всех кнопок.
    uint8_t                     prio;                   // Приоритет задачи, работающий с кнопками.
    uint8_t*                    p_array_sr_reg;         // Указатель на первый элемент массива, который будет выводиться в сдвиговый регистр.
                                                        // Это нужно, чтобы в случае, если, например, наши кнопки весят на 1-м 2-м регистре, то нам придется перезаписывать
                                                        // и первый (т.к. подключены последовательно).
    uint8_t*                    p_button_array;         // Указатель на первый байт, который используются именно кнопками.
};

class buttons_through_shift_register_one_in {
public:
    buttons_through_shift_register_one_in ( buttons_through_shift_register_one_in_cfg* cfg );

    static void task ( void* p_obj );
private:

    const buttons_through_shift_register_one_in_cfg*    const cfg;
};
