#include "buttons_core.h"
#include <string.h>

namespace button {

Base::Base (const BaseCfg *const cfg) : cfg(cfg) {
    USER_OS_STATIC_TASK_CREATE(this->task,
                               "ButtonBase",
                               BUTTON_BASE_TASK_STACK_SIZE,
                               this,
                               this->cfg->taskPrio,
                               this->taskStack,
                               &this->taskStruct);
    
    this->s = new Status[this->cfg->cfgArraySize];
    memset(this->s, 0, sizeof(button::Status) * this->cfg->cfgArraySize);
}

void Base::processPress (uint32_t bNumber) {
    // Сократим запись заранее полученным указателем.
    Status *s = &this->s[bNumber];
    OneButtonCfg *p_st = &this->cfg->cfgArray[bNumber];
    
    //. Если до этого момента кнопка была сброшена.
    if (s->press == false) {
        s->press = true; // Показываем, что нажатие произошло.
        s->bounce = true; // Начинаем проверку на дребезг.
        s->bounceTime = p_st->stabilizationTimeMs; // Устанавливаем время для проверки дребезга контактов.
    } else { // Если мы уже некоторое время держим эту кнопку.
        if (s->bounce == true) { // Если идет проверка на дребезг.
            if (s->bounceTime - this->cfg->taskDelayMs > 0) { /*
                                                               * Если время проверки еще не закончилось
                                                               * (мы от оставшегося времени отнимаем время,
                                                               * прошедшее с предыдущей проверки).
                                                               */
                s->bounceTime -= this->cfg->taskDelayMs;
            } else { // Если время проверки закончилось и, при этом, все это время была зажата, то успех.
                s->bounce = false; // Проверка больше не проводится.
                // Информируем пользователя, если он этого желает
                // (указал семафоры и очереди).
                if (p_st->press.s != nullptr) {
                    USER_OS_GIVE_BIN_SEMAPHORE(*p_st->press.s);
                }
                
                if (p_st->press.q != nullptr) {
                    USER_OS_QUEUE_SEND(*p_st->press.q, &p_st->press.v, 0);
                }
            }
        } else { /*
                  * Если проверка на дребезг уже прошла, при этом клавишу еще держат.
                  */
            if (p_st->longPressDetectionTimeS == 0) { // Если мы не отслеживаем длительное нажатие - выходим.
                return;
            }
            
            if (s->eventLongClick == true) // Если мы уже отдали флаг о том, что нажатие длительное - выходим.
                return;
            
            s->buttonLongClickTime += this->cfg->taskDelayMs; // Прибавляем время, прошедшее с последней проверки.
            
            /*
             * Если прошло времени меньше, чем нужно для того, чтобы кнопка
             * считалась долго зажатой - выходим.
             */
            if (s->buttonLongClickTime < p_st->longPressDetectionTimeS * 1000) {
                return;
            }
            
            s->eventLongClick = true; // Дождались, кнопка долго зажата.
            
            // Информируем пользователя, если он этого желает (указал семафоры и очереди).
            if (p_st->longPress.s != nullptr) {
                USER_OS_GIVE_BIN_SEMAPHORE(*p_st->longPress.s);
            }
            
            if (p_st->longPress.q != nullptr) {
                USER_OS_QUEUE_SEND(*p_st->longPress.q, &p_st->longPress.v, 0);
            }
        }
    }
}

void Base::processNotPress (uint32_t bNumber) {
    Status *s = &this->s[bNumber];
    OneButtonCfg *p_st = &this->cfg->cfgArray[bNumber];
    
    if (s->press == false) {
        return; // Если она и до этого была отпущена - выходим.
    }
    s->press = false;
    
    if (s->bounce == true) { // Если кнопка проходила проверку на дребезг, но не прошла.
        s->bounce = false;
        return;
    }
    
    s->bounceTime = 0;
    s->buttonLongClickTime = 0;
    
    // Если проверку состояния мы все-таки прошли.
    if (s->eventLongClick == true) { // Если произошло длительное нажатие.
        s->eventLongClick = false;
        
        if (p_st->longClick.s != nullptr) {
            USER_OS_GIVE_BIN_SEMAPHORE(*p_st->longClick.s);
        }
        
        if (p_st->longClick.q != nullptr) {
            USER_OS_QUEUE_SEND(*p_st->longClick.q, &p_st->longClick.v, 0);
        }
    }
    
    // Если это было короткое нажатие.
    if (p_st->click.s != nullptr) {
        USER_OS_GIVE_BIN_SEMAPHORE(*p_st->click.s);
    }
    
    if (p_st->click.q != nullptr) {
        USER_OS_QUEUE_SEND(*p_st->click.q, &p_st->click.v, 0);
    }
}

void Base::task (void *obj) {
    button::Base *o = reinterpret_cast<button::Base *>(obj);
    
    while (true) {
        for (uint32_t i = 0; i < o->cfg->cfgArraySize; i++) {
            bool buttonState;
            buttonState = o->cfg->getButtonState(o->cfg->cfgArray[i].id);
            
            if (buttonState == true) {
                o->processPress(i);
            } else {
                o->processNotPress(i);
            }
        }
        
        USER_OS_DELAY_MS(o->cfg->taskDelayMs);
    }
}
    
}
