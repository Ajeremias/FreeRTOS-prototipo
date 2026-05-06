# 🏎️ Protótipo FreeRTOS - Simulação de Carro Autónomo

Bem-vindo ao **Protótipo de Carro Autónomo com FreeRTOS**. Este projecto é uma simulação de ponta de um sistema de controlo de veículos autónomos, integrando o **FreeRTOS** para a gestão de tarefas em tempo real e o **OpenCV** com o **YOLOv3-tiny** para a visão computacional e detecção de objectos em tempo real.

Este protótipo foi desenvolvido para demonstrar a integração do comportamento determinístico do RTOS com cargas de trabalho pesadas de IA num ambiente POSIX (Linux).

---

## 🚀 Como Começar

### 1. Pré-requisitos

Antes de rodar o projecto, certifique-se de que tem o seguinte instalado no seu sistema (testado em Ubuntu/Debian):

#### **Pacotes do Sistema**
```bash
sudo apt update
sudo apt install -y build-essential gcc g++ gdb make pkg-config libopencv-dev
```

#### **Python (para ferramentas AWS e Suporte)**
```bash
sudo apt install -y python3 python3-pip
```

### 2. Clonar o Repositório
Como este projecto utiliza vários sub-módulos para o kernel do FreeRTOS e outras bibliotecas, você **deve** clonar de forma recursiva:

```bash
git clone --recurse-submodules https://github.com/Ajeremias/FreeRTOS-prototipo.git
```

Se já clonou sem os sub-módulos, execute:
```bash
git submodule update --init --recursive
```

### 3. Compilar o Projecto
O protótipo principal está localizado no directório da demo `Posix_GCC`.

```bash
cd FreeRTOS/Demo/Posix_GCC
make clean
make
```

### 4. Executar a Simulação
Execute o binário compilado a partir do mesmo directório:

```bash
./build/rtos_car
```

> [!TIP]
> **Requisito de Visão:** A simulação tentará abrir a `/dev/video0` (Webcam). Se não encontrar nenhuma câmara, o sistema entrará automaticamente no modo de fallback com detecção de obstáculos simulada.

---

## 🛠️ Estrutura do Projecto

- **`FreeRTOS/Demo/Posix_GCC`**: 📍 **Protótipo Principal**. Contém a lógica central, integração com OpenCV e os modelos YOLO.
- **`FreeRTOS/Source`**: O Kernel do FreeRTOS.
- **`FreeRTOS-Plus`**: Bibliotecas suplementares, incluindo as suites TCP/IP e Trace.
- **`tools/`**: Scripts de automação para configuração do AWS IoT e testes CMock.

---

## 🔌 Extensões Recomendadas para o VS Code

Para ter a melhor experiência de desenvolvimento, recomendamos a instalação das seguintes extensões:

1.  **C/C++ Extension Pack** (Microsoft) - Para IntelliSense e depuração (debugging).
2.  **CMake Tools** - Para uma gestão avançada da compilação.
3.  **Python** - Para correr os scripts no directório `tools`.
4.  **Cortex-Debug** - Caso pretenda portar este projecto para hardware ARM (bare-metal).

---

## 🧠 Funcionalidades Principais

- **Escalonamento em Tempo Real**: Tarefas separadas para Visão (Alta Prioridade), Controlo e Monitoria.
- **Detecção de Objectos**: Utiliza o YOLOv3-tiny para detectar pessoas e carros em tempo real.
- **Travagem de Segurança**: Sistema de travagem de emergência automático ao detectar obstáculos.
- **Simulador POSIX**: Permite o desenvolvimento e testes directamente no Linux sem necessidade de hardware dedicado.

---

## 🤝 Fork e Contribuição

1. Faça o **Fork** do projecto.
2. Crie a sua **Branch de Funcionalidade** (`git checkout -b feature/MinhaFuncionalidade`).
3. Faça o **Commit** das suas alterações (`git commit -m 'Adiciona MinhaFuncionalidade'`).
4. Faça o **Push** para a branch (`git push origin feature/MinhaFuncionalidade`).
5. Abra um **Pull Request**.

---

## 📜 Licença
Este projecto baseia-se na distribuição oficial do FreeRTOS e está licenciado sob a **Licença MIT**. Consulte o ficheiro [LICENSE.md](LICENSE.md) para mais detalhes.

---
*Desenvolvido por Ajeremias como parte da série de Protótipos de Sistemas Autónomos com FreeRTOS.*
