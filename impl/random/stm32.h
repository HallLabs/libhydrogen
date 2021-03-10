/* Interface with STM32 model microcontroller. Implements stm_true_random() */
#include "adc.h"

/**
 * @Brief    Function to return a true random number source from STM32 microcontrollers.
 * @Note     IMPORTANT:   pre-release implementation for generation of small batches of unique numbers.
 *                        Not yet random. Do not use for security tokens or encryption.
 * @Note     ADC1 internal temperature sensor must be enabled with default settings.
 * @todo     optimize timing; added extra delays all over while debugging adc timeout
 */

static inline uint32_t stm_true_random(void)
{
    uint32_t temp_entropy = 0;
    for (uint8_t position = 0; position < 32; position += 2)            // acquire 32 bits of entropy
    {
        // Gather random bits from temperature sensor.  TODO: 3rd order difference algorithm to improve randomness.
        HAL_ADC_Start(&hadc1);
        HAL_Delay(10);
        while(HAL_ADC_PollForConversion(&hadc1, 100))
        {
            // Conversion failed; restart the ADC
            HAL_ADC_Stop(&hadc1);
            HAL_Delay(10);
            HAL_ADC_Start(&hadc1);
            HAL_Delay(10);
        }
        uint32_t temp = HAL_ADC_GetValue(&hadc1);
        temp_entropy = (temp_entropy << 2) | (temp & 0b00000011);       // use least 2 bits
        HAL_ADC_Stop(&hadc1);
        HAL_Delay(10); // Ensure readings from temperature sensor are uncorrelated
    }

    return temp_entropy;
}

static int
hydro_random_init(void)
{
    const char       ctx[hydro_hash_CONTEXTBYTES] = { 'h', 'y', 'd', 'r', 'o', 'P', 'R', 'G' };
    hydro_hash_state st;
    uint16_t         ebits = 0;

    hydro_hash_init(&st, ctx, NULL);

    while (ebits < 256) {
        uint32_t r = stm_true_random();
        hydro_hash_update(&st, (const uint32_t *) &r, sizeof r);
        ebits += 32;
    }

    hydro_hash_final(&st, hydro_random_context.state, sizeof hydro_random_context.state);
    hydro_random_context.counter = ~LOAD64_LE(hydro_random_context.state);

    return 0;
}
