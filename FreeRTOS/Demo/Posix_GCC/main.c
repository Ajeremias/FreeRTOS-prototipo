// ========================================================
// FreeRTOS - RTOS para Carro Autónomo (Fase 3)
// Integração com OpenCV + Detecção de Objectos
// ========================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
}

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

using namespace cv;
using namespace cv::dnn;

// ====================== DEFINIÇÕES ======================
typedef enum {
    CMD_ACELERAR,
    CMD_TRAVAR_URGENTE,
    CMD_CURVAR_ESQUERDA,
    CMD_CURVAR_DIREITA,
    CMD_PARAR
} CarCommand;

typedef struct {
    int velocidade;
    int pos_x;
    float angulo;
    int luzes;
    char ultima_acao[64];
} CarroEstado;

static CarroEstado carro = {0, 0, 0.0f, 0, "Parado"};
static SemaphoreHandle_t xCarroMutex = NULL;
static QueueHandle_t xComandoQueue = NULL;

// ====================== TAREFAS ======================

// Tarefa de Visão com OpenCV (Fase 3 real)
void vTarefaVisionOpenCV(void *pvParameters) {
    VideoCapture cap(0);  // 0 = webcam padrão
    if (!cap.isOpened()) {
        printf("[VISION] Erro: Não foi possível abrir a webcam!\n");
        // Fallback para simulação se não houver câmara
        for (;;) {
            if (rand() % 100 < 30) {
                CarCommand cmd = CMD_TRAVAR_URGENTE;
                xQueueSendToFront(xComandoQueue, &cmd, pdMS_TO_TICKS(10));
                printf("[VISION] ⚠️ Obstáculo detectado (simulado)!\n");
            }
            vTaskDelay(pdMS_TO_TICKS(700));
        }
    }

    // Limitar as threads do OpenCV para evitar starvation no RTOS
    cv::setNumThreads(1);

    // Carregar YOLOv3-tiny
    String modelConfiguration = "yolov3-tiny.cfg";
    String modelWeights = "yolov3-tiny.weights";
    Net net;
    try {
        net = readNetFromDarknet(modelConfiguration, modelWeights);
        net.setPreferableBackend(DNN_BACKEND_OPENCV);
        net.setPreferableTarget(DNN_TARGET_OPENCL);  // Aceleração gráfica (GPU)
        printf("[VISION] YOLOv3-tiny carregado com sucesso (OpenCL GPU)!\n");
    } catch (Exception& e) {
        printf("[VISION] Erro ao carregar YOLO: %s\n", e.what());
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    Mat frame, blob;
    std::vector<String> outNames = net.getUnconnectedOutLayersNames();
    std::vector<Mat> outs;

    printf("[VISION] Webcam aberta com sucesso. Iniciando detecção...\n");

    for (;;) {
        // Yield antes da leitura bloqueante da webcam para prevenir starvation de outras tarefas
        vTaskDelay(pdMS_TO_TICKS(10));

        cap >> frame;
        if (frame.empty()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Yield antes da inferência pesada
        vTaskDelay(pdMS_TO_TICKS(10));

        // Otimização: Criar blob com resolução reduzida (320x320 em vez de 416x416) para inferência mais rápida
        blobFromImage(frame, blob, 1/255.0, Size(320, 320), Scalar(0,0,0), true, false);
        net.setInput(blob);
        net.forward(outs, outNames);

        bool obstaculo_detectado = false;
        int id_detectado = -1;

        // Processar saídas do YOLO
        for (size_t i = 0; i < outs.size(); ++i) {
            float* data = (float*)outs[i].data;
            for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols) {
                Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
                Point classIdPoint;
                double confidence;
                minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
                
                // Se a confiança for > 50% E for uma pessoa (0) ou carro (2)
                if (confidence > 0.5 && (classIdPoint.x == 0 || classIdPoint.x == 2)) {
                    obstaculo_detectado = true;
                    id_detectado = classIdPoint.x;
                    break;
                }
            }
            if(obstaculo_detectado) break;
        }

        if (obstaculo_detectado) {
            CarCommand cmd = CMD_TRAVAR_URGENTE;
            xQueueSendToFront(xComandoQueue, &cmd, pdMS_TO_TICKS(5));
            const char* nome = (id_detectado == 0) ? "PESSOA" : "CARRO";
            printf("[VISION] 🚨 %s detetado(a)! Travagem urgente.\n", nome);
        }

        // Mostrar imagem (opcional - pode ser comentado)
        // imshow("Visão do Carro", frame);
        // if (waitKey(1) == 27) break;   // ESC para sair

        vTaskDelay(pdMS_TO_TICKS(300));   // ~3 FPS para não sobrecarregar a CPU
    }
}

// Tarefa de Controlo
void vTarefaControlo(void *pvParameters) {
    CarCommand cmd;
    for (;;) {
        if (xQueueReceive(xComandoQueue, &cmd, pdMS_TO_TICKS(100)) == pdPASS) {
            xSemaphoreTake(xCarroMutex, portMAX_DELAY);

            switch (cmd) {
                case CMD_TRAVAR_URGENTE:
                    carro.velocidade = (carro.velocidade > 30) ? carro.velocidade - 40 : 0;
                    snprintf(carro.ultima_acao, 64, "TRAVAGEM URGENTE");
                    printf("[CONTROLO] 🚨 Travagem de Emergência Activada!\n");
                    break;

                case CMD_ACELERAR:
                    if (carro.velocidade < 100) carro.velocidade += 10;
                    carro.pos_x += 5;
                    snprintf(carro.ultima_acao, 64, "ACELERAR");
                    break;

                case CMD_CURVAR_ESQUERDA:
                    carro.angulo -= 18.0f;
                    snprintf(carro.ultima_acao, 64, "CURVAR ESQUERDA");
                    break;

                case CMD_CURVAR_DIREITA:
                    carro.angulo += 18.0f;
                    snprintf(carro.ultima_acao, 64, "CURVAR DIREITA");
                    break;

                case CMD_PARAR:
                    carro.velocidade = 0;
                    snprintf(carro.ultima_acao, 64, "PARAR");
                    break;
            }
            xSemaphoreGive(xCarroMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

// Monitor
void vTarefaMonitor(void *pvParameters) {
    for (;;) {
        xSemaphoreTake(xCarroMutex, portMAX_DELAY);
        printf("=== CARRO AUTÓNOMO === Vel: %3d km/h | Pos: %4d | Ângulo: %6.1f° | Última: %s\n",
               carro.velocidade, carro.pos_x, carro.angulo, carro.ultima_acao);
        xSemaphoreGive(xCarroMutex);
        vTaskDelay(pdMS_TO_TICKS(1800));
    }
}

int main(void) {
    printf("\n=== FreeRTOS - Carro Autónomo Fase 3 (OpenCV + Visão) ===\n\n");

    xCarroMutex = xSemaphoreCreateMutex();
    xComandoQueue = xQueueCreate(20, sizeof(CarCommand));

    if (xCarroMutex == NULL || xComandoQueue == NULL) {
        printf("Erro ao criar Mutex ou Queue!\n");
        while(1);
    }

    xTaskCreate(vTarefaVisionOpenCV, "Vision",  16384, NULL, 6, NULL);  // prioridade alta
    xTaskCreate(vTarefaControlo,     "Control", 1024, NULL, 5, NULL);
    xTaskCreate(vTarefaMonitor,      "Monitor", 1024, NULL, 2, NULL);

    vTaskStartScheduler();

    for(;;);
    return 0;
}

extern "C" {
void vApplicationMallocFailedHook( void )
{
    printf("Malloc Failed\n");
    for( ;; );
}

void vAssertCalled( const char * const pcFileName, unsigned long ulLine )
{
    printf("Assert %s:%lu\n", pcFileName, ulLine);
    for( ;; );
}

void vApplicationIdleHook( void ) {}
void vApplicationTickHook( void ) {}
void vApplicationDaemonTaskStartupHook( void ) {}

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ 4096 ];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = 4096;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ 8192 ];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = 8192;
}
}
