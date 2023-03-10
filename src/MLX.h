#include <Arduino.h>
#include <Wire.h>

#define IMG_WIDTH 32
#define IMG_HEIGHT 24

#define ADDR 0x33

/**
 * Viewer protocol:
 * Total packet format is <0x69><CMD><LEN><LEN><DATA>
 */
class MLX
{
public:
    const short ADDR_STATUS = 0x8000;
    const short ADDR_CR1 = 0x800D;
    const char *chars = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

    struct
    {
        unsigned en_subpage_mode : 1;
        unsigned : 1;
        unsigned en_data_hold : 1;
        unsigned en_subpage_repeat : 1;
        unsigned select_subpage : 3;
        unsigned refresh_rate : 3;
        unsigned resolution : 2;
        unsigned use_chessboard_pattern : 1;
    } cr1;

    struct
    {
        unsigned subpage : 3;
        unsigned has_data : 1;
        unsigned end_data_overwrite : 1;
    } status;

    TwoWire *wire;
    int16_t img[IMG_WIDTH * IMG_HEIGHT] = {0};

    MLX(TwoWire *wire)
    {
        this->wire = wire;
    }

    void read_register(uint16_t reg_addr, uint8_t *target, size_t len = 1, bool reverse = true)
    {
        TwoWire &w = *this->wire;

        w.beginTransmission(ADDR);

        w.write(reg_addr >> 8);
        w.write(reg_addr & 0xFF);
        if (w.endTransmission(false) != 0) // Do not release bus
        {
            Serial.println("No ack...");
            return; // Sensor did not ACK
        }
        w.requestFrom(ADDR, len * 2);
        w.readBytes(target, len * 2);

        if (!reverse)
            return;

        // Reverse bytes pairs in the target buffer
        for (int i = 0; i < len * 2; i += 2)
        {
            uint8_t temp = target[i];
            target[i] = target[i + 1];
            target[i + 1] = target[i];
        }
    }

    void write_register(uint16_t reg_addr, uint8_t *target, size_t len = 1)
    {
        TwoWire &w = *this->wire;

        w.beginTransmission(ADDR);
        w.write(reg_addr >> 8);
        w.write(reg_addr & 0xFF);

        for (int i = 0; i < len * 2; i += 2)
        {
            w.write(target[i + 1]);
            w.write(target[i]);
        }
        w.endTransmission();
    }

    bool init()
    {
        delay(80);
        // Serial.println("Wire starting!");
        wire->begin(21, 22, 1000000u);

        cr1.en_subpage_mode = 1;
        cr1.en_data_hold = 0;
        cr1.en_subpage_repeat = 0;
        cr1.refresh_rate = 3;
        cr1.resolution = 2;
        cr1.use_chessboard_pattern = 0;
        // Serial.println(*(uint16_t *)&cr1, BIN);

        write_register(ADDR_CR1, (uint8_t *)&cr1);
        read_register(ADDR_CR1, (uint8_t *)&cr1);

        // Serial.println("=========== CONFIG READBACK ===========");
        // Serial.println(*(uint16_t *)&cr1, BIN);
        // Serial.printf("Refresh: %d\n", cr1.refresh_rate);
        // Serial.printf("Subpage Mode: %d\n", cr1.en_subpage_mode);
        // Serial.printf("Use chessboard: %d\n", cr1.use_chessboard_pattern);
        // Serial.printf("Resolution: %d\n", cr1.resolution);
        // Serial.printf("Raw: %d\n", *(uint16_t *)&cr1);
        return true;
    }

    bool has_img()
    {
        read_register(ADDR_STATUS, (uint8_t *)&status);
        if (status.has_data)
        {
            // Serial.printf("DATA GET - subpage: %d\n", status.subpage);
            status.has_data = 0;
            write_register(ADDR_STATUS, (uint8_t *)&status);
            return true;
        }
        return false;
    }

    long read_img()
    {
        long start = millis();
        for (int i = status.subpage; i < IMG_HEIGHT; i += 2)
        {
            read_register(0x0400 + i * 0x20, (uint8_t *)&img[i * 0x20], IMG_WIDTH);
        }
        return millis() - start;
    }

    void print_img()
    {
        int16_t img_min = img[0];
        int16_t img_max = img[0];
        int16_t range = 0;

        for (int i = 0; i < IMG_WIDTH; i++)
        {
            img_min = min(img[i], img_min);
            img_max = max(img[i], img_max);
        }
        range = img_max - img_min;
        if (!range)
            return;

        Serial.printf("IMG RANGED %d %d %d\n", img_max, img_min, range);
        Serial.println((img[6] - img_min) * 69 / range);
        for (int row = 0; row < IMG_HEIGHT; row++)
        {
            for (int col = 0; col < IMG_WIDTH; col++)
            {
                int16_t px_val = img[row * IMG_WIDTH + col];
                int16_t znorm = px_val - img_min;

                if (znorm < range * 2 / 4)
                    znorm = 0;

                int chars_idx = constrain(znorm * 69 / range, 0, 69);
                Serial.write(chars[chars_idx]);
            }
            Serial.println();
        }
    }

    void tx_debug_msg(const char *msg)
    {
        size_t len = strlen(msg);
        Serial.write(0xFE);
        Serial.write(0x01);
        Serial.write((uint8_t *)&len, 2);
        Serial.print(msg);
    }

    void tx_current_image()
    {
        size_t len = IMG_WIDTH * IMG_HEIGHT * 2;

        Serial.write(0xFE);
        Serial.write(0x00);
        Serial.write((uint8_t *)&len, 2);
        Serial.write((uint8_t *)&img, len);
    }
};
