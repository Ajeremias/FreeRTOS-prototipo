***************************************************************************************************************************
  Este projecto foi desenvolvido em contexto academico, para fins de apredizagem.
***************************************************************************************************************************

Protótipo FreeRTOS - Simulação de Carro Autónomo

 Este projecto é uma simulação de um sistema operativo em tempo real de controlo de veículos autónomos, integrando o **FreeRTOS** para a gestão de tarefas em tempo real e o **OpenCV** com o **YOLOv3-tiny** para a visão computacional e detecção de objectos em tempo real.

Este protótipo foi desenvolvido para demonstrar a integração do comportamento determinístico do RTOS com cargas de trabalho pesadas de IA num ambiente POSIX (Linux).



### 1. Pré-requisitos

Antes de rodar o projecto, certifique-se de que tem o seguinte instalado no seu sistema (testado em Ubuntu/Debian):

 *Pacotes do Sistema*
```bash
sudo apt update
sudo apt install -y build-essential gcc g++ gdb make pkg-config libopencv-dev
```

 *Python (para ferramentas AWS e Suporte)*
 
```bash
sudo apt install -y python3 python3-pip
```

 *2. Clone o Repositório do FreeRTOS*

***git clone --recurse-submodules https://github.com/Ajeremias/FreeRTOS-prototipo.git***

Se já clonou sem os sub-módulos, execute:
```bash
git submodule update --init --recursive
```

*3. Compilar o Projecto*
O protótipo principal está localizado no directório da demo `Posix_GCC`.


***cd FreeRTOS/Demo/Posix_GCC
make clean
make***


 *4. Executar a Simulação*
Execute o binário compilado a partir do mesmo directório:

```
./build/rtos_car
```

***************************************************************************************************************************
 **Aviso**
> **Requisito de Visão:** A simulação tentará abrir a `/dev/video0` (Webcam). Se não encontrar nenhuma câmara, o sistema entrará automaticamente no modo de fallback com detecção de obstáculos simulada.

***************************************************************************************************************************

## Estrutura do Projecto

- **`FreeRTOS/Demo/Posix_GCC`**: Contém a lógica central, integração com OpenCV e os modelos YOLO.
- **`FreeRTOS/Source`**: O Kernel do FreeRTOS.
- **`FreeRTOS-Plus`**: Bibliotecas suplementares, incluindo as suites TCP/IP e Trace.
- **`tools/`**: Scripts de automação para configuração do AWS IoT e testes CMock.


## Extensões Recomendadas para o VS Code

Para ter a melhor experiência de desenvolvimento, recomendamos a instalação das seguintes extensões:

1.  **C/C++ Extension Pack** (Microsoft) - Para IntelliSense e depuração (debugging).
2.  **CMake Tools** - Para uma gestão avançada da compilação.
3.  **Python** - Para correr os scripts no directório `tools`.





***************************************************************************************************************************
**DESCRIÇÃO DO PROJECTO**
***************************************************************************************************************************
## 1. Resumo
Este trabalho apresenta o projeto, implementação e otimização de um sistema de controlo para um protótipo de veículo autónomo, utilizando o sistema operativo de tempo-real (RTOS) FreeRTOS. A investigação foca-se na convergência entre a computação determinística e a visão computacional baseada em Deep Learning (YOLOv3-tiny). O sistema foi validado em ambiente de simulação POSIX, apresentando uma arquitetura multi-tarefa robusta que assegura a integridade do controlo cinemático mesmo sob elevada carga computacional. Foram aplicadas técnicas de aceleração por hardware via OpenCL e otimização de concorrência para mitigar problemas de inversão de prioridade e starvation.

## 2. Introdução
O crescimento dos veículos autónomos exige sistemas operativos capazes de garantir determinismo, previsibilidade e resposta em tempo real a eventos críticos, como a deteção de obstáculos.
Este projeto consistiu no desenvolvimento de um RTOS leve baseado no FreeRTOS para controlo de um carro autónomo em ambiente simulado. O foco principal foi a implementação e otimização do kernel e das tarefas de controlo em tempo real, em vez da parte física ou mecânica do veículo.
Todo o sistema foi desenvolvido e testado em simuladores (POSIX/Linux port e QEMU), permitindo validar o comportamento do sistema operativo sem necessidade de hardware real. O RTOS gerencia tarefas críticas como arranque, aceleração, travagem de emergência e manobras básicas de curva.
O projeto demonstra a capacidade do FreeRTOS em gerir múltiplas tarefas com diferentes prioridades, garantindo que funções de segurança (ex: travagem) tenham resposta previsível mesmo sob carga.

## 3. Delimitação do Tema e Problema

### 3.1. Delimitação do Tema
O tema circunscreve-se ao desenvolvimento de software de sistema para veículos autónomos, especificamente no que toca à integração de modelos de deteção de objetos (YOLO) dentro de um ambiente gerido por um kernel RTOS (FreeRTOS), operando em arquiteturas x86 (simulação) e ARM (emulação bare-metal).

### 3.2. Problema
O problema central reside na **latência e no determinismo**. Como garantir que as tarefas críticas de controlo e travagem de emergência cumpram os seus *deadlines* quando o processador é submetido a tarefas de inferência de redes neuronais que são inerentemente pesadas e, por vezes, bloqueantes a nível de I/O? O desafio é evitar que a visão computacional cause *starvation* (fome de recursos) nas tarefas de segurança do veículo.

## 4. Objetivos

### 4.1. Objetivo Geral
Conceber uma arquitetura de software de tempo-real capaz de detetar obstáculos via inteligência artificial e executar comandos de controlo cinemático com garantias de segurança e performance.

### 4.2. Objetivos Específicos
*   Criar e configurar um projeto FreeRTOS com múltiplas tarefas em ambiente Ubuntu.
*   Implementar tarefas dedicadas para aquisição de sensores, controlo de velocidade e atuação (motores e direção).
*   Implementar um escalonamento de tarefas baseado em prioridades fixas com preempção.
*   Desenvolver um mecanismo de IPC (Inter-Process Communication) resiliente via filas.
*   Integrar o modelo YOLOv3-tiny otimizado para hardware gráfico através da API DNN do OpenCV.
*   Analisar e mitigar o impacto da inferência na latência do sistema operativo.
*   Documentar os resultados de desempenho comparativos entre processamento em CPU e GPU.

## 5. Revisão Bibliográfica

### 5.1. Fundamentos de RTOS e FreeRTOS
Sistemas de Tempo-Real são caracterizados por terem a sua correção dependente não apenas do resultado lógico, mas também do tempo em que esse resultado é entregue (BARRY, 2016). O FreeRTOS providencia as primitivas necessárias — semáforos, mutexes e queues — que permitem a sincronização de tarefas. A utilização de Mutexes com herança de prioridade é vital para evitar a inversão de prioridade em sistemas onde tarefas de diferentes níveis partilham recursos comuns.

### 5.2. Deep Learning em Sistemas Embutidos: YOLO
O algoritmo YOLO (*You Only Look Once*) revolucionou a deteção de objetos por tratar a deteção como um problema de regressão único, permitindo taxas de FPS elevadas. A variante *Tiny* simplifica a arquitetura da rede (menos camadas convolucionais), tornando-a viável para processadores com recursos limitados (REDMON; FARHADI, 2018).

### 5.3. Aceleração via OpenCL e DNN
O módulo DNN do OpenCV permite a execução de redes neuronais em diversos *backends*. A utilização de OpenCL (*Open Computing Language*) permite o desvio de tarefas paralelas massivas para o GPU, seguindo o modelo de computação heterogénea, o que é essencial para libertar o CPU para as tarefas de controlo do kernel FreeRTOS.

## 6. Metodologia

### 6.1. Abordagem Experimental
O desenvolvimento seguiu uma metodologia incremental:
1.  **Prototipagem em POSIX**: Utilização do simulador Linux para validar a lógica de IA e concorrência sem dependência de hardware físico imediato.
2.  **Desenho de Tarefas**:
    *   `vTarefaVision`: Executa o pipeline de visão (320x320 resolution).
    *   `vTarefaControlo`: Gere o estado do veículo (Velocidade, Ângulo).
    *   `vTarefaMonitor`: Log e telemetria.
3.  **Otimização de Hardware**: Implementação de `DNN_TARGET_OPENCL` e limitação de concorrência interna do OpenCV para garantir o determinismo do RTOS.

### 6.2. Ferramentas e Instrumentação
*   **Compilador**: GCC e ARM-none-eabi-gcc.
*   **Emulador**: QEMU (Stellaris LM3S6965).
*   **Análise**: Gprof para perfilagem de CPU e FreeRTOS Run-Time Stats para análise de carga de tarefas.

## 7. Desenvolvimento e Resultados

### 7.1. Integração YOLO e Filtragem de Dados
Foi implementado um filtro de classes no pós-processamento da rede neuronal. O sistema ignora deteções irrelevantes e foca-se exclusivamente em `Person` (ID 0) e `Car` (ID 2). Esta filtragem reduz a carga de decisão da tarefa de controlo.

### 7.2. Análise de Performance e Starvation
Os testes revelaram que a inferência do YOLO pode consumir até 90% da capacidade de um núcleo de CPU. Através da introdução de `vTaskDelay` e da aceleração por GPU, conseguiu-se reduzir a ocupação do CPU pela tarefa de visão para níveis que permitem a execução estável das tarefas de controlo, mantendo a latência de resposta abaixo dos limites críticos de segurança.


##  Licença
Este projecto baseia-se na distribuição oficial do FreeRTOS e está licenciado sob a **Licença MIT**. Consulte o ficheiro [LICENSE.md](LICENSE.md) para mais detalhes.

---
*Sistemas Operativo - SISTEMA OPERATIVOS PARA CARROS AUTONOMOS*
