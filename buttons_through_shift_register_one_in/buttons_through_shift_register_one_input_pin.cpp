#include "buttons_through_shift_register_one_input_pin.h"

#ifdef MODULE_BUTTONS_THROUGH_SHIFT_REGISTER_ONE_INPUT_PIN_ENABLED

#include <string.h>

ButtonsThroughShiftRegisterOneInputPin::ButtonsThroughShiftRegisterOneInputPin ( ButtonsThroughShiftRegisterOneInputPinCfg* cfg ) : cfg( cfg ) {}

void ButtonsThroughShiftRegisterOneInputPin::init() {
	USER_OS_STATIC_TASK_CREATE( this->task, "b_sr", BUTTONS_THROUGH_SHIFT_REGISTER_ONE_IN_TASK_STACK_SIZE, ( void* )this, this->cfg->prio, this->taskStack, &this->taskStruct );
	uint32_t b_count = this->cfg->pinCount;
	// Обязательно очищаем поля статуса всех клавиш.
	for ( uint32_t l = 0; l < b_count; l++ ) {
		this->cfg->pinCfgArray[ l ].status->press					= false;
		this->cfg->pinCfgArray[ l ].status->bounce					= false;
		this->cfg->pinCfgArray[ l ].status->eventLongClick			= false;
		this->cfg->pinCfgArray[ l ].status->bounceTime				= 0;
		this->cfg->pinCfgArray[ l ].status->buttonLongClickTime		= 0;
	}
}

// Выбрать нужную кнопку через сдвиговом регистре (подать на ее вывод 0, а на остальные 1).
void ButtonsThroughShiftRegisterOneInputPin::selectButton ( const uint32_t &bNumber ) {		// Значение bNumber можно не проверять, т.к. это внутренний метод и на уровне выше уже проверено.
	memset( this->cfg->pointingButtonArray, 0, this->cfg->srBufferByteCount );					// Не опрашиваемые в данный момент кнопки == 0.
	this->cfg->pointingButtonArray[ this->cfg->pinCfgArray[ bNumber ].byte ] |=
		1 << this->cfg->pinCfgArray[ bNumber ].bit;											// Ставим 1 на ножке (т.к. вывод подтянут к 0, то при нажатой клавише мы сразу полочим 1 на входе).
	this->cfg->srObj->write();																	// Перезаписываем все в сдвиговом регистре.
}

// Метод возвращает true - если клавиша нажата, false, если нет.
bool ButtonsThroughShiftRegisterOneInputPin::checkPress ( void ) {
	return this->cfg->pinInObj->read();
}

// Обработать нажатую клавишу.
void ButtonsThroughShiftRegisterOneInputPin::processPress ( const uint32_t &bNumber ) {
	srOneInButtonStatusStruct* s		= this->cfg->pinCfgArray[ bNumber ].status; // Для удобства записи.
	sr_one_in_button_item_cfg*		p_st	= &this->cfg->pinCfgArray[ bNumber ];

	if ( s->press == false ) {	// Если до этого момента кнопка была сброшена.
		s->press		= true;																 // Показываем, что нажатие произошло.
		s->bounce		= true;																 // Начинаем проверку на дребезг.
		s->bounceTime	= p_st->stabildDelayMs;														// Устанавливаем время для проверки дребезга контактов.
	} else {					// Если мы уже некоторое время держим эту кнопку.
		if ( s->bounce == true ) {		// Если идет проверка на дребезг.
			if ( s->bounceTime - this->cfg->delayMs > 0 ) {	// Если время проверки еще не закончилось (мы от оставшегося времени отнимаем время, прошедшее с предыдущей проверки).
				s->bounceTime -= this->cfg->delayMs;
			} else {											// Если время проверки закончилось и, при этом, все это время была зажата, то успех.
				s->bounce			= false;													// Проверка больше не проводится.
				// Информируем пользователя, если он этого желает (указал семафоры и очереди).
				if ( p_st->sPress != nullptr )
					USER_OS_GIVE_BIN_SEMAPHORE( *p_st->sPress );
				if ( p_st->qPress != nullptr )
					USER_OS_QUEUE_SEND( *p_st->qPress, &p_st->vPress, 0 );					// Кладем в очеред без ожидания.
			}
		} else {						// Если проверка на дребезг уже прошла, при этом клавишу еще держат.
			if ( p_st->delayMsLongPress == 0 ) return;												// Если мы не отслеживаем длительное нажатие - выходим.
			if ( s->eventLongClick == true ) return;											// Если мы уже отдали флаг о том, что нажатие длительное - выходим.
			s->buttonLongClickTime += this->cfg->delayMs;									// Прибавляем время, прошедшее с последней проверки.
			if ( s->buttonLongClickTime < p_st->delayMsLongPress ) return;						// Если прошло времени меньше, чем нужно для того, чтобы кнопка считалась долго зажатой - выходим.
			s->eventLongClick = true;														 // Дождались, кнопка долго зажата.
			// Информируем пользователя, если он этого желает (указал семафоры и очереди).
			if ( p_st->sStartLongPress != nullptr )
				USER_OS_GIVE_BIN_SEMAPHORE( *p_st->sStartLongPress );
			if ( p_st->qStartLongPress != nullptr )
				USER_OS_QUEUE_SEND( *p_st->qStartLongPress, &p_st->vStartLongPress, 0 );
		}
	}
}

// Обработать отпущенную клавишу.
void ButtonsThroughShiftRegisterOneInputPin::processNotPress ( const uint32_t &bNumber ) {
	srOneInButtonStatusStruct* s		= this->cfg->pinCfgArray[ bNumber ].status;
	sr_one_in_button_item_cfg*		p_st	= &this->cfg->pinCfgArray[ bNumber ];

	if ( s->press == false ) return;															// Если она и до этого была отпущена - выходим.
	s->press = false;

	if ( s->bounce == true ) {																	// Если кнопка проходила проверку на дребезг, но не прошла.
		s->bounce					= false;
		return;
	}

	s->bounceTime			= 0;
	s->buttonLongClickTime	= 0;

	// Если проверку состояния мы все-таки прошли.
	if ( s->eventLongClick == true ) {														// Если произошло длительное нажатие.
		s->eventLongClick		 = false;
		// Информируем пользователя, если он этого желает (указал семафоры и очереди).
		if ( p_st->sReleaseLongClick != nullptr )
			USER_OS_GIVE_BIN_SEMAPHORE( *p_st->sReleaseLongClick );
		if ( p_st->qReleaseLongClick != nullptr )
			USER_OS_QUEUE_SEND( *p_st->qReleaseLongClick, &p_st->vReleaseLongClick, 0 );
		return;
	}

	// Если это было короткое нажатие.
	if ( p_st->sReleaseClick != nullptr )
		USER_OS_GIVE_BIN_SEMAPHORE( *p_st->sReleaseClick );
	if ( p_st->qReleaseClick != nullptr )
		USER_OS_QUEUE_SEND( *p_st->qReleaseClick, &p_st->vReleaseClick, 0 );
}

void ButtonsThroughShiftRegisterOneInputPin::task ( void* p_obj ) {
	ButtonsThroughShiftRegisterOneInputPin* o = ( ButtonsThroughShiftRegisterOneInputPin* )p_obj;
	while ( true ) {
		if ( o->cfg->mutex != nullptr )
			USER_OS_TAKE_MUTEX( *o->cfg->mutex, portMAX_DELAY );

		for ( uint32_t b_l = 0; b_l < o->cfg->pinCount; b_l++ ) {
			o->selectButton( b_l );
			if ( o->checkPress() == true ) {
				o->processPress( b_l );
			} else {
				o->processNotPress( b_l );
			}
		}

		if ( o->cfg->mutex != nullptr )
			USER_OS_GIVE_MUTEX( *o->cfg->mutex );

		USER_OS_DELAY_MS( o->cfg->delayMs );
	}
}

#endif
