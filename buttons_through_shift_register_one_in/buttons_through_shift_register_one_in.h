#pragma once

#include "mk_hardware_interfaces_pin.h"
#include "module_shift_register.h"
#include "user_os.h"

#define BUTTONS_THROUGH_SHIFT_REGISTER_ONE_IN_TASK_STACK_SIZE       100

// Флаги состояния клавиши (одной).
struct sr_one_in_button_status_struct {
    bool        press;                  // Флаг состояния нажатия до текущей проверки (окончание предыдущей).
    bool        bounce;                 // Флаг состояния проверки на дребезг: true - идет (еще идет), false - пройдена успешно.
    bool        event_bounce;           // Флаг события "прошла проверку дребезга (была нажата)": true - нажали, false - нет (не прошла проверку на дребезг или была отпущена раньше, чем та завершилась).
    bool        event_long_click;       // Флаг события "произошло длительное нажатие": true - произошло, false - нет.
    uint8_t     bounce_time;            // Оставшееся время до окончания проверки на дребезг (в мс).
    uint16_t    button_long_click_time; // Сколько прошло времени с момента нажатия клавиши (идет начиная с проверки дребезга).

};

// Структура для каждого вывода, подключенного через сдвиговый регистр.
struct sr_one_in_button_item_cfg {
    // Положение кнопкаи в сдвиговом регистре.
    const uint8_t                       byte;                                       // Бит в сдвиговом регистре, на котором весит кнопка (счет с 0).
    const uint8_t                       bit;                                        // Байт, в котором находится кнопка (бит кнопки), счет с 0.
                                                                                    // Причем 0-й байт - это point_button_array.
                                                                                    // А он может быть уже 3-м или каким еще относительно point_array_reg.

    // Отслеживание дребезга контактов.
    const uint8_t                             ms_stabil;                            // Колличество мс, для стабилизации нажатия (защита от дребезга).

    // Отслеживание долгого нажатия.
    const uint16_t                            dl_delay_ms;                          // Время нажатия, после которого кнопка считается долго зажатой. В миллисекундах.
                                                                                    // Если тут 0, то долгое нажатие не детектируется.
    // Кнопку нажали (прошла проверку дребезга).
          USER_OS_STATIC_BIN_SEMAPHORE*       const s_press;                        // Семафор, который будет отдан сразу же после стабилизации нажатия. Если указан.
          USER_OS_STATIC_QUEUE*               const q_press;                        // Очередь, в которую будет положено значение сразу же после нажатия клавиши (с учетом стабилизации от дребезга).
    const uint8_t                             v_press;                              // Значение, которое будет положено в q_press.

    // Кнопку долго держат (еще не отпустили).
          USER_OS_STATIC_BIN_SEMAPHORE*       const s_start_long_press;
          USER_OS_STATIC_QUEUE*               const q_start_long_press;             // Очередь, в которую будет записано значение сразу же, как только пройдет время, после которого кнопка будет считаться удерживаемой долго, но до ее отпускания.
    const uint8_t                             v_start_long_press;


    // Кнопку отпустили.
          USER_OS_STATIC_BIN_SEMAPHORE*       const s_release_click;                // Семафор, который будет отдан сразу же после отпуска кнопки (при коротком). Если указан.
          USER_OS_STATIC_BIN_SEMAPHORE*       const s_release_long_click;           // Симафор, который будет отдан сразу же, после отпускания кнопки с длительным нажатием.
          USER_OS_STATIC_BIN_SEMAPHORE*       const s_start_long_press;             // Симафор, который будет отдан сразу же, как только пройдет время, после которого кнопка будет считаться удерживаемой долго, но до ее отпускания.
          USER_OS_STATIC_QUEUE*               const q_release_click;                // Очередь, в которую будет положено значение сразу же после отпускания кнопки (короткого).
    const uint8_t                             v_release_click;
          USER_OS_STATIC_QUEUE*               const q_release_release_long_click;   // Очередь, в которую будет записано значение сразу после отпускания (длительного).
    const uint8_t                             v_release_release_long_click;

    sr_one_in_button_status_struct*           status;                               // Статус клавиши (используется внутренней машиной состояний).
};

struct buttons_through_shift_register_one_in_cfg {
          pin_base*                   const p_in_pin;                               // Общий для всех кнопок вывод. Обязательно должен быть подтянут к +!
          module_shift_register*      const p_sr;                                   // HC595 или аналог. Шина данных идет через подключенный сдвиговый регистр.
          sr_one_in_button_item_cfg*  const p_pin_conf_array;                       // Указатель на массив с описанием выводов.
    const uint32_t                    pin_count;                                    // Колличество выводов в массиве.
    const uint32_t                    delay_ms;                                     // Пауза между опросами всех кнопок.
    const uint8_t                     prio;                                         // Приоритет задачи, работающий с кнопками.
          uint8_t*                    const p_button_array;                         // Указатель на первый байт, который используются именно кнопками.
    const uint8_t                     sr_buffer_byte_count;                         // Количество байт, которые уходят буферу кнопок.
};

class buttons_through_shift_register_one_in {
public:
    buttons_through_shift_register_one_in ( buttons_through_shift_register_one_in_cfg* cfg );

    static void task ( void* p_obj );

private:
    void select_button          ( const uint32_t &b_number );                       // Выбрать нужную кнопку через сдвиговом регистре (подать на ее вывод 0, а на остальные 1).
    bool check_press            ( void );                                           // Метод возвращает true - если клавиша нажата, false, если нет.

    void process_press          ( const uint32_t &b_number );                       // Обработка клавиши в случае, если она в данный момент нажата.
    void process_not_press      ( const uint32_t &b_number );                       // Если отпущена.


    const buttons_through_shift_register_one_in_cfg*    const cfg;

    // Для создания задачи.
    USER_OS_STATIC_STACK_TYPE           task_stack[ BUTTONS_THROUGH_SHIFT_REGISTER_ONE_IN_TASK_STACK_SIZE ] = { 0 };
    USER_OS_STATIC_TASK_STRUCT_TYPE     task_struct = USER_OS_STATIC_TASK_STRUCT_INIT_VALUE;

};
