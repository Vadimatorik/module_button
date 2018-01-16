ifndef MODULE_BUT_OPTIMIZATION
	MODULE_BUT_OPTIMIZATION = -g3 -O0
endif

#**********************************************************************
# module_button
#**********************************************************************
BUT_H_FILE				:= $(shell find module_button/ -maxdepth 3 -type f -name "*.h" )
BUT_CPP_FILE			:= $(shell find module_button/ -maxdepth 3 -type f -name "*.cpp" )
BUT_DIR					:= $(shell find module_button/ -maxdepth 3 -type d -name "*" )
BUT_PATH				:= $(addprefix -I, $(BUT_DIR))
BUT_OBJ_FILE			:= $(addprefix build/obj/, $(BUT_CPP_FILE))
BUT_OBJ_FILE			:= $(patsubst %.cpp, %.o, $(BUT_OBJ_FILE))

build/obj/module_button/%.o:	module_button/%.cpp
	@echo [CPP] $<
	@mkdir -p $(dir $@)
	@$(CPP) $(CPP_FLAGS) $(MK_INTER_PATH) $(BUT_PATH) $(USER_CFG_PATH) $(FREE_RTOS_PATH) $(SH_PATH) $(MODULE_BUT_OPTIMIZATION) -c $< -o $@

# Добавляем к общим переменным проекта.
PROJECT_PATH			+= $(BUT_PATH)
PROJECT_OBJ_FILE		+= $(BUT_OBJ_FILE)