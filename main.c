#include <SoapySDR/Device.h>
#include <SoapySDR/Errors.h>
#include <SoapySDR/Formats.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 1000000
#define FREQUENCY 800000000
#define TX_DELAY_NS 4000000LL
#define PULSE_SAMPLES 4000
#define TX_AMPLITUDE 2000
#define PAUSE_SAMPLES 300000
#define PERIOD_SAMPLES (PULSE_SAMPLES + PAUSE_SAMPLES)
#define RX_GAIN 30
#define TX_GAIN 40

static volatile sig_atomic_t running = 1;

static void stop(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static void fill_tx(int16_t *data, size_t count, size_t *position)
{
    for (size_t i = 0; i < count; i++) {
        size_t n = *position;
        // Прямоугольный импульс в базовой полосе: I = Q = const.
        int16_t value = n < PULSE_SAMPLES ? TX_AMPLITUDE : 0;
        data[2 * i] = value;
        data[2 * i + 1] = value;
        *position = (n + 1) % PERIOD_SAMPLES;
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--help")) {
        printf("Приём:    %s rx USB_URI [новый_файл.pcm]\nПередача: %s tx USB_URI\n", argv[0], argv[0]);
        printf("URI: iio_info -s; остановка: Ctrl+C; RX по умолчанию пишет rx_week56.pcm; существующий файл не заменяется.\n");
        return 0;
    }
    if (argc < 3 || argc > 4 || (strcmp(argv[1], "rx") && strcmp(argv[1], "tx")) ||
        (argc == 4 && !strcmp(argv[1], "tx"))) {
        printf("Приём:    ./build/sdr rx USB_URI [новый_файл.pcm]\n");
        printf("Передача: ./build/sdr tx USB_URI\n");
        return 1;
    }
    int transmit = !strcmp(argv[1], "tx");
    const char *output = argc == 4 ? argv[3] : "rx_week56.pcm";
    SoapySDRKwargs args = {0};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    SoapySDRKwargs_set(&args, "uri", argv[2]);
    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");
    SoapySDRKwargs_set(&args, "loopback", "0");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);
    if (!sdr) {
        fprintf(stderr, "Pluto: %s\n", SoapySDRDevice_lastError());
        return 1;
    }

    SoapySDRStream *rx = NULL, *tx = NULL;
    int16_t *rx_data = NULL, *tx_data = NULL;
    FILE *file = NULL;
    int rx_active = 0, tx_active = 0, result = 1;
    int warmup = 128, previous_count = 0;
    long long previous_time = 0;
    size_t channel = 0, position = 0, total_rx = 0, total_tx = 0;
    size_t gaps = 0, next_log = SAMPLE_RATE;
    size_t rx_mtu = 0, tx_mtu = 0;
    const char *stage = "настройка RX";
    signal(SIGINT, stop);
    signal(SIGTERM, stop);

    if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, SAMPLE_RATE) ||
        SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, FREQUENCY, NULL) ||
        SoapySDRDevice_setGainMode(sdr, SOAPY_SDR_RX, 0, 0) ||
        SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, RX_GAIN)) goto cleanup;
    if (transmit) {
        stage = "настройка TX";
        if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, SAMPLE_RATE) ||
            SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, FREQUENCY, NULL) ||
            SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 0, TX_GAIN)) goto cleanup;
    }

    stage = "создание RX";
    rx = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, &channel, 1, NULL);
    if (!rx) goto cleanup;
    rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rx);
    if (!rx_mtu || rx_mtu > SIZE_MAX / (2 * sizeof(int16_t))) goto cleanup;
    stage = "буфер RX";
    rx_data = calloc(2 * rx_mtu, sizeof(int16_t));
    if (!rx_data) { perror("Буфер RX"); goto cleanup; }

    if (transmit) {
        stage = "создание TX";
        tx = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, &channel, 1, NULL);
        if (!tx) goto cleanup;
        tx_mtu = SoapySDRDevice_getStreamMTU(sdr, tx);
        if (!tx_mtu) goto cleanup;
        tx_data = calloc(2 * rx_mtu, sizeof(int16_t));
        if (!tx_data) { perror("Буфер TX"); goto cleanup; }
    } else {
        stage = "создание нового файла RX";
        file = fopen(output, "wbx");
        if (!file) { perror(output); goto cleanup; }
    }

    stage = "запуск RX";
    if (SoapySDRDevice_activateStream(sdr, rx, 0, 0, 0)) goto cleanup;
    rx_active = 1;
    if (transmit) {
        stage = "запуск TX";
        if (SoapySDRDevice_activateStream(sdr, tx, 0, 0, 0)) goto cleanup;
        tx_active = 1;
    }
    printf("Режим %s, 800 МГц, 1 МГц, CS16. MTU RX=%zu, TX=%zu\n",
           transmit ? "TX" : "RX", rx_mtu, tx_mtu);
    printf("Остановка: Ctrl+C.\n");
    printf("Прогрев: 128 RX-буферов. RX gain=%d; TX gain=%d (шкала драйвера).\n",
           RX_GAIN, TX_GAIN);
    if (transmit) printf("Сигнал: 4 мс I=Q=2000, 300 мс нулей; период 304 мс.\n");
    else printf("RX сохраняет новый файл: %s\n", output);
    fflush(stdout);

    while (running) {
        void *rx_buffers[] = {rx_data};
        int flags = 0;
        long long time_ns = 0;
        stage = "чтение RX";
        int count = SoapySDRDevice_readStream(sdr, rx, rx_buffers, rx_mtu,
                                             &flags, &time_ns, 400000);
        if (!running) break;
        if (count <= 0 || (size_t)count > rx_mtu) {
            fprintf(stderr, "Ошибка RX: %d (%s)\n", count, SoapySDR_errToStr(count));
            goto cleanup;
        }
        if (!(flags & SOAPY_SDR_HAS_TIME)) {
            fprintf(stderr, "RX: нет временной метки\n");
            goto cleanup;
        }
        if (warmup > 0) { warmup--; continue; }
        if (previous_count) {
            long long gap = time_ns - previous_time - (long long)previous_count * 1000;
            if (llabs(gap) > 10000) {
                gaps++;
                fprintf(stderr, "RX: разрыв %lld нс\n", gap);
            }
        } else {
            printf("Первый RX: %d отсчётов, метка %lld нс\n", count, time_ns);
            fflush(stdout);
        }
        previous_time = time_ns;
        previous_count = count;
        total_rx += (size_t)count;

        if (transmit) {
            fill_tx(tx_data, (size_t)count, &position);
            size_t sent = 0;
            while (sent < (size_t)count && running) {
                size_t length = (size_t)count - sent;
                if (length > tx_mtu) length = tx_mtu;
                const void *tx_buffers[] = {tx_data + 2 * sent};
                flags = SOAPY_SDR_HAS_TIME;
                long long tx_time = time_ns + TX_DELAY_NS + (long long)sent * 1000;
                stage = "запись TX";
                int written = SoapySDRDevice_writeStream(sdr, tx, tx_buffers, length,
                                                         &flags, tx_time, 400000);
                if (written <= 0) {
                    if (!running) break;
                    fprintf(stderr, "Ошибка TX: %d (%s)\n", written, SoapySDR_errToStr(written));
                    goto cleanup;
                }
                if ((size_t)written > length) goto cleanup;
                sent += (size_t)written;
                total_tx += (size_t)written;
            }
        } else {
            stage = "запись файла RX";
            if (fwrite(rx_data, 2 * sizeof(int16_t), (size_t)count, file) != (size_t)count) {
                perror(output);
                goto cleanup;
            }
        }
        if (total_rx >= next_log) {
            printf("RX=%zu, TX=%zu, метка=%lld нс, разрывов=%zu\n",
                   total_rx, total_tx, time_ns, gaps);
            fflush(stdout);
            next_log = total_rx + SAMPLE_RATE;
        }
    }
    result = (transmit ? total_tx : total_rx) ? 0 : 1;
    stage = "остановка до получения данных";

cleanup:
    if (result) fprintf(stderr, "Сбой: %s. SoapySDR: %s\n", stage, SoapySDRDevice_lastError());
    if (tx_active) SoapySDRDevice_deactivateStream(sdr, tx, 0, 0);
    if (rx_active) SoapySDRDevice_deactivateStream(sdr, rx, 0, 0);
    if (tx) SoapySDRDevice_closeStream(sdr, tx);
    if (rx) SoapySDRDevice_closeStream(sdr, rx);
    if (file && fclose(file)) { perror(output); result = 1; }
    free(tx_data);
    free(rx_data);
    SoapySDRDevice_unmake(sdr);
    if (transmit) printf("TX: драйвер принял %zu комплексных отсчётов\n", total_tx);
    else printf("RX: получено %zu комплексных отсчётов\n", total_rx);
    printf("Разрывов RX по меткам: %zu\n", gaps);
    return result;
}
