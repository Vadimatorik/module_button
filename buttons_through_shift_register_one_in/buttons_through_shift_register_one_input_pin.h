#pragma once

#ifdef MODULE_BUTTONS_THROUGH_SHIFT_REGISTER_ONE_INPUT_PIN_ENABLED

#include "shift_register.h"
#include "mc_hardware_interfaces_pin.h"
#include "user_os.h"

#define BUTTONS_THROUGH_SHIFT_REGISTER_ONE_IN_TASK_STACK_SIZE		100

// Флаги состояния клавиши (одной).
struct srOneInButtonStatusStruct {
	bool								press;										// Флаг состояния нажатия до текущей проверки (окончание предыдущей).
	bool								bounce;										// Флаг состояния проверки на дребезг: true - идет (еще идет), false - пройдена успешно.
	bool								eventLongClick;								// Флаг события "произошло длительное нажатие": true - произошло, false - нет.
	uint8_t								bounceTime;									// Оставшееся время до окончания проверки на дребезг (в мс).
	uint16_t							buttonLongClickTime;						// Сколько прошло времени с момента нажатия клавиши (идет начиная с проверки дребезга).
};

// Структура для каждого вывода, подключенного через сдвиговый регистр.
struct sr_one_in_button_item_cfg {
	// Положение кнопкаи в сдвиговом регистре.
	const uint8_t						byte;										// Бит в сдвиговом регистре, на котором весит кнопка (счет с 0).
	const uint8_t						bit;										// Байт, в котором находится кнопка (бит кнопки), счет с 0.
																					// Причем 0-й байт - это point_button_array.
																					// А он может быть уже 3-м или каким еще относительно point_array_reg.

	// Отслеживание дребезга контактов.
	const uint8_t						stabildDelayMs;								// Колличество мс, для стабилизации нажатия (защита от дребезга).

	// Отслеживание долгого нажатия.
	const uint16_t						delayMsLongPress;							// Время нажатия, после которого кнопка считается долго зажатой. В миллисекундах.
																					// Если тут 0, то долгое нажатие не детектируется.
	// Кнопку нажали (прошла проверку дребезга).
	USER_OS_STATIC_BIN_SEMAPHORE*		const sPress;								// Семафор, который будет отдан сразу же после стабилизации нажатия. Если указан.
	USER_OS_STATIC_QUEUE*				const qPress;								// Очередь, в которую будет положено значение сразу же после нажатия клавиши (с учетом стабилизации от дребезга).
	const uint8_t						vPress;										// Значение, которое будет положено в q_press.

	// Кнопку долго держат (еще не отпустили).
	USER_OS_STATIC_BIN_SEMAPHORE*		const sStartLongPress;
	USER_OS_STATIC_QUEUE*				const qStartLongPress;						// Очередь, в которую будет записано значение сразу же, как только пройдет время, после которого кнопка будет считаться удерживаемой долго, но до ее отпускания.
	const uint8_t						vStartLongPress;

	// Кнопку отпустили после длительного нажатия.
	USER_OS_STATIC_BIN_SEMAPHORE*		const sReleaseLongClick;					// Симафор, который будет отдан сразу же, после отпускания кнопки с длительным нажатием.
	USER_OS_STATIC_QUEUE*				const qReleaseLongClick;					// Очередь, в которую будет записано значение сразу после отпускания (длительного).
	const uint8_t						vReleaseLongClick;

	// Кнопку отпустили после короткого нажатия.
	USER_OS_STATIC_BIN_SEMAPHORE*		const sReleaseClick;						// Семафор, который будет отдан сразу же после отпуска кнопки (при коротком). Если указан.
	USER_OS_STATIC_QUEUE*				const qReleaseClick;						// Очередь, в которую будет записано значение сразу после отпускания (короткого).
	const uint8_t						vReleaseClick;

	srOneInButtonStatusStruct*			status;										// Статус клавиши (используется внутренней машиной состояний).
};

struct ButtonsThroughShiftRegisterOneInputPinCfg {
	PinBase*							const pinInObj;								// Общий для всех кнопок вывод. Обязательно должен быть подтянут к +!
	ShiftRegister*						const srObj;								// HC595 или аналог. Шина данных идет через подключенный сдвиговый регистр.
	sr_one_in_button_item_cfg*			const pinCfgArray;							// Указатель на массив с описанием выводов.
	const uint32_t						pinCount;									// Колличество выводов в массиве.
	const uint32_t						delayMs;									// Пауза между опросами всех кнопок.
	const uint8_t						prio;										// Приоритет задачи, работающий с кнопками.
	uint8_t*							const pointingButtonArray;					// Указатель на первый байт, который используются именно кнопками
																					// (внутри массива, который отсылает сдвиговый регистр).
	const uint8_t						srBufferByteCount;							// Количество байт, которые уходят буферу кнопок.

	// Данный mutex используется в том случае, если на одном SPI весят несколько устройств.
	// Он забирается перед CS и возвращается после считывания данных со всхода мк (о нажатии кнопки).
	USER_OS_STATIC_MUTEX*			 const mutex;
};

class ButtonsThroughShiftRegisterOneInputPin {
public:
	ButtonsThroughShiftRegisterOneInputPin ( ButtonsThroughShiftRegisterOneInputPinCfg* cfg );
			void init ( void );
	static	void task ( void* p_obj );

private:
	void selectButton			( const uint32_t &bNumber );						// Выбрать нужную кнопку через сдвиговом регистре (подать на ее вывод 0, а на остальные 1).
	bool checkPress				( void );											// Метод возвращает true - если клавиша нажата, false, если нет.

	void processPress			( const uint32_t &bNumber );						// Обработка клавиши в случае, если она в данный момент нажата.
	void processNotPress		( const uint32_t &bNumber );						// Если отпущена.

	const ButtonsThroughShiftRegisterOneInputPinCfg*	const cfg;

	// Для создания задачи.
	USER_OS_STATIC_STACK_TYPE			taskStack[ BUTTONS_THROUGH_SHIFT_REGISTER_ONE_IN_TASK_STACK_SIZE ] = { 0 };
	USER_OS_STATIC_TASK_STRUCT_TYPE		taskStruct;
};

#endif
