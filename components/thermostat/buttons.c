#include "buttons.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_timer.h"

#define DEBOUNCE_TIME_US 20000
#define TIMER_SEQ_OP_US 300000

gpio_num_t BUTTON_DOWN_PIN = GPIO_NUM_8;
gpio_num_t BUTTON_UP_PIN = GPIO_NUM_3;

atomic_bool button_down_pressed = false;
atomic_bool button_up_pressed = false;

static esp_timer_handle_t button_down_debounce_timer;
static esp_timer_handle_t button_up_debounce_timer;

void IRAM_ATTR button_up_handler(void* data)
{
	gpio_intr_disable(BUTTON_UP_PIN);
	esp_timer_start_once(button_up_debounce_timer, DEBOUNCE_TIME_US);
}

void IRAM_ATTR button_down_handler(void* data)
{
	gpio_intr_disable(BUTTON_DOWN_PIN);
	esp_timer_start_once(button_down_debounce_timer, DEBOUNCE_TIME_US);
}

void IRAM_ATTR button_debounce_handler(void* data)
{
	gpio_num_t pin = *(gpio_num_t*)data;
	int level = gpio_get_level(pin);
	if (level == 0) {
		if (pin == BUTTON_DOWN_PIN) {
			atomic_store(&button_down_pressed, true);
			esp_timer_start_once(button_down_debounce_timer, TIMER_SEQ_OP_US);
		}
		else {
			atomic_store(&button_up_pressed, true);
			esp_timer_start_once(button_up_debounce_timer, TIMER_SEQ_OP_US);
		}
	} else {
		gpio_intr_enable(pin);
	}
}

void buttons_init(void)
{
	ESP_ERROR_CHECK(gpio_install_isr_service(0));

	gpio_config_t button_cfg = {
		.pin_bit_mask = (1ULL << BUTTON_DOWN_PIN),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_NEGEDGE,
	};
	ESP_ERROR_CHECK(gpio_config(&button_cfg));
	ESP_ERROR_CHECK(gpio_isr_handler_add(
		BUTTON_DOWN_PIN,
		button_down_handler,
		NULL
	));

	button_cfg.pin_bit_mask = (1ULL << BUTTON_UP_PIN);
	ESP_ERROR_CHECK(gpio_config(&button_cfg));
	ESP_ERROR_CHECK(gpio_isr_handler_add(
		BUTTON_UP_PIN,
		button_up_handler,
		NULL
	));

	esp_timer_create_args_t args = {
		.callback = button_debounce_handler,
		.arg = &BUTTON_DOWN_PIN,
		.dispatch_method = ESP_TIMER_TASK,
		.skip_unhandled_events = true,
	};
	ESP_ERROR_CHECK(esp_timer_create(&args, &button_down_debounce_timer));
	args.arg = &BUTTON_UP_PIN;
	ESP_ERROR_CHECK(esp_timer_create(&args, &button_up_debounce_timer));
}
