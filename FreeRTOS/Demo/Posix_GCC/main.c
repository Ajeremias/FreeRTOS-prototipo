/**
 * ============================================================================
 * @file main.c
 * @brief Sistema de Controle de Carro Autónomo com FreeRTOS e OpenCV.
 * 
 * Este programa simula um sistema de tempo real para um veículo autónomo.
 * Utiliza o FreeRTOS para gestão de tarefas e o OpenCV com YOLOv3-tiny para
 * detecção de obstáculos via visão computacional.
 * 
 * @author Antigravity (Otimizado por IA)
 * @date 2026
 * ============================================================================
 */

#include <stdio.h>      // Biblioteca padrão de entrada e saída
#include <stdlib.h>     // Biblioteca padrão (rand, malloc, etc.)
#include <string.h>     // Manipulação de strings
#include <stdint.h>     // Tipos de dados inteiros de largura fixa
#include <vector>       // Container vector do C++

// Inclusão das bibliotecas do FreeRTOS
extern "C" {
#include "FreeRTOS.h"   // Definições core do FreeRTOS
#include "task.h"       // Gestão de tarefas (tasks)
#include "queue.h"      // Comunicação entre tarefas via filas
#include "semphr.h"     // Sincronização via semáforos/mutexes
}

// Inclusão da biblioteca OpenCV para processamento de imagem e IA
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

// ============================================================================
// CONFIGURAÇÃO DO SISTEMA (CONSTANTES)
// ============================================================================

// Tamanhos das stacks (pilhas) para cada tarefa em "words"
#define STACK_SIZE_VISION    16384  // Stack maior para processamento de imagem
#define STACK_SIZE_CONTROL   2048   // Stack média para lógica de controle
#define STACK_SIZE_MONITOR   1024   // Stack pequena para monitorização

// Prioridades das tarefas (quanto maior o número, maior a prioridade)
#define PRIORITY_VISION      ( tskIDLE_PRIORITY + 6 ) // Prioridade máxima: Segurança
#define PRIORITY_CONTROL     ( tskIDLE_PRIORITY + 5 ) // Prioridade alta: Execução
#define PRIORITY_MONITOR     ( tskIDLE_PRIORITY + 2 ) // Prioridade baixa: Apenas exibição

// Parâmetros da Rede Neural YOLO
#define YOLO_CONF_THRESHOLD  0.5    // Confiança mínima (50%) para considerar uma detecção
#define YOLO_INPUT_SIZE      320    // Resolução de entrada da rede (320x320)
#define PERSON_CLASS_ID      0      // ID da classe 'pessoa' no dataset COCO
#define CAR_CLASS_ID         2      // ID da classe 'carro' no dataset COCO

// Configurações de Comunicação e Tempo
#define QUEUE_LENGTH         20     // Capacidade máxima da fila de comandos
#define COMMAND_TIMEOUT      pdMS_TO_TICKS(100) // Tempo de espera na leitura da fila (100ms)
#define VISION_PERIOD        pdMS_TO_TICKS(300) // Intervalo entre frames de visão (300ms)
#define MONITOR_PERIOD       pdMS_TO_TICKS(2000) // Intervalo de atualização da telemetria (2s)

// ============================================================================
// DEFINIÇÕES DE TIPOS E ESTRUTURAS
// ============================================================================

/**
 * @brief Enumeração de comandos possíveis para o veículo.
 */
typedef enum {
    CMD_ACELERAR,        // Aumenta a velocidade
    CMD_TRAVAR_URGENTE,  // Reduz drasticamente a velocidade
    CMD_CURVAR_ESQUERDA, // Altera o ângulo para a esquerda
    CMD_CURVAR_DIREITA,  // Altera o ângulo para a direita
    CMD_PARAR            // Para o veículo completamente
} CarCommand;

/**
 * @brief Estrutura que representa o estado atual do carro.
 */
typedef struct {
    int velocidade;      // Velocidade atual em km/h
    int pos_x;           // Posição linear simulada
    float angulo;        // Ângulo de direção
    int luzes;           // Estado das luzes (não implementado nesta fase)
    char ultima_acao[64]; // Descrição textual da última manobra realizada
} CarroEstado;

// ============================================================================
// VARIÁVEIS GLOBAIS (PROTEGIDAS)
// ============================================================================

// Estado global do carro (inicializado como parado)
static CarroEstado carro = {0, 0, 0.0f, 0, (char*)"Parado"};

// Handler do Mutex para garantir exclusão mútua ao aceder ao estado do carro
static SemaphoreHandle_t xCarroMutex = NULL;

// Handler da Fila de Comandos para comunicação entre Visão e Controle
static QueueHandle_t xComandoQueue = NULL;

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

/**
 * @brief Inicializa a rede YOLOv3-tiny a partir dos ficheiros de configuração e pesos.
 * @param net Referência para o objeto Net do OpenCV.
 * @return true se carregada com sucesso, false caso contrário.
 */
static bool bInitYOLO(cv::dnn::Net &net) {
    const cv::String modelConfiguration = "yolov3-tiny.cfg";
    const cv::String modelWeights = "yolov3-tiny.weights";

    try {
        // Carrega o modelo Darknet
        net = cv::dnn::readNetFromDarknet(modelConfiguration, modelWeights);
        // Configura o backend para OpenCV
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        // Tenta usar OpenCL para aceleração via GPU se disponível
        net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL); 
        printf("[VISION] YOLOv3-tiny carregado com sucesso!\n");
        return true;
    } catch (const cv::Exception& e) {
        printf("[VISION] Erro ao carregar YOLO: %s\n", e.what());
        return false;
    }
}

/**
 * @brief Processa as saídas das camadas da rede neural para identificar obstáculos críticos.
 * @param outs Vetor com as matrizes de saída da rede.
 * @param out_id Ponteiro para armazenar o ID do objeto detectado.
 * @return true se um obstáculo (pessoa/carro) for detectado com confiança suficiente.
 */
static bool bDetectarObstaculo(const std::vector<cv::Mat>& outs, int &out_id) {
    for (const auto& output : outs) {
        float* data = (float*)output.data;
        // Itera sobre todas as detecções encontradas no frame
        for (int j = 0; j < output.rows; ++j, data += output.cols) {
            // Scores começam no índice 5 da linha de dados
            cv::Mat scores = output.row(j).colRange(5, output.cols);
            cv::Point classIdPoint;
            double confidence;
            // Encontra a classe com maior score/confiança
            cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            
            // Verifica se a confiança supera o threshold configurado
            if (confidence > YOLO_CONF_THRESHOLD) {
                // Filtra apenas por pessoas ou carros (obstáculos na estrada)
                if (classIdPoint.x == PERSON_CLASS_ID || classIdPoint.x == CAR_CLASS_ID) {
                    out_id = classIdPoint.x;
                    return true;
                }
            }
        }
    }
    return false;
}

// ============================================================================
// TAREFAS DO FREERTOS
// ============================================================================

/**
 * @brief TAREFA: Visão Artificial (OpenCV)
 * Captura frames da webcam, processa-os via YOLO e envia comandos de emergência
 * se detectar perigo iminente.
 */
void vTarefaVisionOpenCV(void *pvParameters) {
    (void)pvParameters; // Parâmetro não utilizado
    cv::VideoCapture cap(0); // Tenta abrir a primeira webcam disponível
    cv::dnn::Net net;
    cv::Mat frame, blob;
    std::vector<cv::Mat> outs;
    
    // Força o OpenCV a usar apenas 1 thread para não interferir no scheduler do RTOS
    cv::setNumThreads(1);

    // Verifica se a câmara abriu ou se o modelo carregou
    if (!cap.isOpened() || !bInitYOLO(net)) {
        printf("[VISION] Modo de Fallback Ativado (Câmara ou YOLO indisponível)\n");
        // Loop de simulação caso o hardware falhe
        for (;;) {
            if (rand() % 100 < 15) { // 15% de chance de obstáculo simulado
                CarCommand cmd = CMD_TRAVAR_URGENTE;
                // Envia para a frente da fila (alta prioridade)
                xQueueSendToFront(xComandoQueue, &cmd, 0);
                printf("[VISION] ⚠️ Obstáculo simulado detectado!\n");
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); // Espera 1 segundo
        }
    }

    // Obtém os nomes das camadas de saída para a inferência
    std::vector<cv::String> outNames = net.getUnconnectedOutLayersNames();
    printf("[VISION] Sistema de Visão Iniciado.\n");

    for (;;) {
        cap >> frame; // Captura um frame
        if (frame.empty()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Converte o frame em um Blob para processamento pela rede neural
        cv::dnn::blobFromImage(frame, blob, 1/255.0, cv::Size(YOLO_INPUT_SIZE, YOLO_INPUT_SIZE), cv::Scalar(0,0,0), true, false);
        net.setInput(blob);
        net.forward(outs, outNames); // Realiza a inferência (forward pass)

        int id_detectado = -1;
        // Verifica se há obstáculos perigosos
        if (bDetectarObstaculo(outs, id_detectado)) {
            CarCommand cmd = CMD_TRAVAR_URGENTE;
            // Envia comando de travagem imediata para a fila
            xQueueSendToFront(xComandoQueue, &cmd, pdMS_TO_TICKS(10));
            
            const char* nome = (id_detectado == PERSON_CLASS_ID) ? "PESSOA" : "CARRO";
            printf("[VISION] 🚨 %s detetado! Travagem urgente enviada.\n", nome);
        }

        vTaskDelay(VISION_PERIOD); // Libera o CPU para outras tarefas
    }
}

/**
 * @brief TAREFA: Lógica de Controle
 * Processa comandos vindos da fila e atualiza o estado do veículo de forma segura.
 */
void vTarefaControlo(void *pvParameters) {
    (void)pvParameters;
    CarCommand cmd;

    for (;;) {
        // Tenta receber um comando da fila (bloqueia até 100ms se estiver vazia)
        if (xQueueReceive(xComandoQueue, &cmd, COMMAND_TIMEOUT) == pdPASS) {
            // Tenta obter o Mutex para alterar o estado do carro (exclusão mútua)
            if (xSemaphoreTake(xCarroMutex, portMAX_DELAY) == pdTRUE) {
                switch (cmd) {
                    case CMD_TRAVAR_URGENTE:
                        // Reduz velocidade de forma brusca
                        carro.velocidade = (carro.velocidade > 30) ? carro.velocidade - 40 : 0;
                        strncpy(carro.ultima_acao, "TRAVAGEM URGENTE", 64);
                        printf("[CONTROLO] 🚨 Travagem de Emergência!\n");
                        break;

                    case CMD_ACELERAR:
                        // Aumenta velocidade suavemente
                        if (carro.velocidade < 100) carro.velocidade += 10;
                        carro.pos_x += 5;
                        strncpy(carro.ultima_acao, "ACELERAR", 64);
                        break;

                    case CMD_CURVAR_ESQUERDA:
                        carro.angulo -= 15.0f;
                        strncpy(carro.ultima_acao, "CURVAR ESQUERDA", 64);
                        break;

                    case CMD_CURVAR_DIREITA:
                        carro.angulo += 15.0f;
                        strncpy(carro.ultima_acao, "CURVAR DIREITA", 64);
                        break;

                    case CMD_PARAR:
                        carro.velocidade = 0;
                        strncpy(carro.ultima_acao, "PARAR", 64);
                        break;
                }
                // Libera o Mutex após a atualização
                xSemaphoreGive(xCarroMutex);
            }
        }
        // Pequena pausa para estabilidade do scheduler
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * @brief TAREFA: Monitorização (Telemetria)
 * Exibe periodicamente no console o estado atual do veículo.
 */
void vTarefaMonitor(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        // Solicita acesso ao estado do carro (leitura segura)
        if (xSemaphoreTake(xCarroMutex, portMAX_DELAY) == pdTRUE) {
            printf("\n--- TELEMETRIA ---\n");
            printf("Velocidade: %3d km/h\n", carro.velocidade);
            printf("Posição X:  %4d m\n", carro.pos_x);
            printf("Ângulo:     %6.1f°\n", carro.angulo);
            printf("Estado:     %s\n", carro.ultima_acao);
            printf("------------------\n");
            xSemaphoreGive(xCarroMutex);
        }
        vTaskDelay(MONITOR_PERIOD); // Aguarda o próximo ciclo de exibição
    }
}

// ============================================================================
// FUNÇÃO PRINCIPAL (ENTRY POINT)
// ============================================================================

int main(void) {
    printf("\n=== FreeRTOS Autonomous Car Simulator ===\n");
    printf("Iniciando recursos...\n");

    // Cria o Mutex para proteção de dados globais
    xCarroMutex = xSemaphoreCreateMutex();
    // Cria a Fila para comunicação assíncrona entre tarefas
    xComandoQueue = xQueueCreate(QUEUE_LENGTH, sizeof(CarCommand));

    // Validação da criação dos recursos do RTOS
    if (xCarroMutex == NULL || xComandoQueue == NULL) {
        fprintf(stderr, "Falha crítica: Não foi possível criar Mutex ou Queue!\n");
        return 1;
    }

    // Criação das Tarefas do Sistema
    // xTaskCreate(Função, Nome, StackSize, Parâmetro, Prioridade, Handle)
    xTaskCreate(vTarefaVisionOpenCV, "Vision",  STACK_SIZE_VISION,  NULL, PRIORITY_VISION,  NULL);
    xTaskCreate(vTarefaControlo,     "Control", STACK_SIZE_CONTROL, NULL, PRIORITY_CONTROL, NULL);
    xTaskCreate(vTarefaMonitor,      "Monitor", STACK_SIZE_MONITOR, NULL, PRIORITY_MONITOR, NULL);

    // Inicia o Escalonador de Tarefas (o controle passa para o FreeRTOS aqui)
    vTaskStartScheduler();

    // Este ponto só é atingido se houver falta de memória para iniciar o scheduler
    for(;;);
    return 0;
}

// ============================================================================
// GANCHOS DO FREERTOS (HOOKS)
// ============================================================================

extern "C" {
/**
 * @brief Chamado quando uma tentativa de alocação de memória (malloc) falha.
 */
void vApplicationMallocFailedHook( void ) {
    fprintf(stderr, "Erro: Falha na alocação de memória (Malloc Failed)\n");
    for( ;; );
}

/**
 * @brief Chamado em caso de erro interno do kernel (configASSERT).
 */
void vAssertCalled( const char * const pcFileName, unsigned long ulLine ) {
    fprintf(stderr, "Assert falhou em %s:%lu\n", pcFileName, ulLine);
    for( ;; );
}

// Ganchos vazios (hooks opcionais)
void vApplicationIdleHook( void ) {}
void vApplicationTickHook( void ) {}
void vApplicationDaemonTaskStartupHook( void ) {}

/**
 * @brief Fornece memória estática para a Tarefa Idle do sistema.
 */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize ) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ 4096 ];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = 4096;
}

/**
 * @brief Fornece memória estática para a Tarefa de Timer do sistema.
 */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize ) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ 4096 ];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = 4096;
}
}
