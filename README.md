# InfraMed IOT - Dispositivo de Monitoramento de Medicação

**Dispositivo IoT baseado em ESP32 para pesagem e registro de medicações, integrado em tempo real com a API de monitoramento de pacientes InfraMed.**

---

<p align="center">
  <img src="https://img.shields.io/badge/Status-Concluído-brightgreen" alt="Status do Projeto: Concluído">
  <img src="https://img.shields.io/badge/Plataforma-PlatformIO-orange?logo=platformio" alt="PlatformIO">
  <img src="https://img.shields.io/badge/Framework-Arduino-00979D?logo=arduino&logoColor=white" alt="Arduino Framework">
  <img src="https://img.shields.io/badge/Hardware-ESP32-E7352C?logo=espressif&logoColor=white" alt="ESP32">
</p>

---

## 📋 Sumário

- [📖 Sobre o Projeto](#-sobre-o-projeto)
- [✨ Principais Funcionalidades](#-principais-funcionalidades)
- [⚙️ Hardware Utilizado](#-hardware-utilizado)
- [🛠️ Bibliotecas e Tecnologias](#-bibliotecas-e-tecnologias)
- [🔄 Fluxo de Funcionamento](#-fluxo-de-funcionamento)
- [🚀 Como Executar](#-como-executar)
- [💡 Contexto do Projeto](#-contexto-do-projeto)
- [✍️ Autor](#-autor)

---

## 📖 Sobre o Projeto

O **InfraMed IoT** é o componente de hardware do ecossistema InfraMed, materializando a ponte entre o mundo físico e o sistema de gestão hospitalar. Este projeto consiste em um dispositivo embarcado, construído com um microcontrolador **ESP32**, projetado para automatizar e garantir a precisão no processo de administração de medicamentos.

O dispositivo guia o profissional de saúde através de um fluxo simples e intuitivo para registrar a pesagem de medicamentos, associando-a diretamente a um atendimento específico. Os dados coletados são enviados via Wi-Fi para o backend [InfraMed](https://github.com/matheus05dev/BackendMonitoramentoPacientes), garantindo que as informações estejam sempre atualizadas e centralizadas, reduzindo erros manuais e otimizando o tempo da equipe.

Este projeto é a ponta de lança do monitoramento em tempo real, demonstrando a aplicação prática de IoT no setor da saúde para criar um ambiente mais seguro e eficiente.

---

## ✨ Principais Funcionalidades

- **Interface de Usuário Intuitiva:** Um display OLED e um teclado matricial guiam o usuário passo a passo.
- **Máquina de Estados Robusta:** O fluxo de operação é controlado por uma máquina de estados que gerencia a entrada de dados e a pesagem.
- **Pesagem de Precisão:** Utiliza um sensor de célula de carga com o módulo HX711 para medições de peso precisas.
- **Conectividade Wi-Fi:** Conecta-se à rede local para comunicação direta com o servidor backend.
- **Integração com API REST:** Envia os dados de pesagem (valor, código do medicamento, duração) via requisição HTTP POST para o endpoint `/api/leituras/atendimento/{id}`.
- **Feedback Visual:** LEDs de status (verde e vermelho) e mensagens no display informam o usuário sobre o status da conexão, envio de dados e operações.
- **Funcionalidades da Balança:** Inclui funções essenciais como "Tarar" (zerar a balança) e a opção de reiniciar o processo.

---

## ⚙️ Hardware Utilizado

| Componente                | Descrição                                                   | Pinos (ESP32)                                       |
| ------------------------- | ----------------------------------------------------------- | --------------------------------------------------- |
| **ESP32 DevKit**          | Microcontrolador principal com Wi-Fi integrado.             | -                                                   |
| **Display OLED SSD1306**  | Tela de 128x64 pixels para a interface com o usuário (I2C). | `SDA (21)`, `SCL (22)`                              |
| **Módulo HX711**          | Amplificador para a célula de carga (balança).              | `DOUT (15)`, `CLK (2)`                              |
| **Célula de Carga**       | Sensor que mede o peso.                                     | Conectada ao HX711                                  |
| **Teclado Matricial 4x3** | Para entrada de dados (IDs, códigos, comandos).             | Linhas: `27, 26, 25, 33` <br> Colunas: `13, 12, 14` |
| **LED Verde**             | Indica sucesso na operação (ex: dado enviado).              | `18`                                                |
| **LED Vermelho**          | Indica erro ou estado de espera (ex: conectando ao Wi-Fi).  | `19`                                                |

---

## 🛠️ Bibliotecas e Tecnologias

A escolha das tecnologias foi focada na robustez e na facilidade de desenvolvimento para a plataforma ESP32:

- **PlatformIO:** Ambiente de desenvolvimento profissional para sistemas embarcados.
- **Arduino Framework:** Simplifica a programação do microcontrolador com uma vasta gama de bibliotecas.
- **WiFi.h & HTTPClient.h:** Para conectividade de rede e realização de requisições HTTP para a API.
- **ArduinoJson:** Essencial para criar e serializar o payload JSON enviado ao servidor.
- **Adafruit GFX & Adafruit SSD1306:** Bibliotecas para controle e renderização de gráficos e textos no display OLED.
- **HX711.h:** Para comunicação com o módulo da célula de carga e obtenção das leituras de peso.
- **Keypad.h:** Facilita a leitura de dados do teclado matricial.

---

## 🔄 Fluxo de Funcionamento

O dispositivo opera em uma máquina de estados clara e sequencial:

1.  **Inicialização:**

    - O sistema inicializa os componentes (Display, LEDs, Balança).
    - Conecta-se à rede Wi-Fi configurada. O LED vermelho fica aceso durante a tentativa de conexão.
    - Após a conexão, o LED vermelho apaga e o sistema está pronto.

2.  **ETAPA 1: Inserir ID do Atendimento:**

    - O display solicita o `ID do Atendimento`.
    - O usuário digita o ID no teclado e pressiona `#` para confirmar.

3.  **ETAPA 2: Inserir Código da Medicação:**

    - O display solicita o `Código da Medicação`.
    - O usuário digita o código e pressiona `#` para confirmar.

4.  **ETAPA 3: Inserir Duração:**

    - O display solicita a `Duração do Procedimento` em minutos.
    - O usuário digita o valor e pressiona `#` para confirmar.

5.  **ETAPA 4: Pesagem e Envio:**
    - O sistema entra no modo de pesagem, exibindo o peso atual em tempo real.
    - O display mostra as opções: `*` para enviar, `2` para tarar, `1` para reiniciar.
    - Ao pressionar `*`, o dispositivo monta um payload JSON e o envia via **HTTP POST** para a API InfraMed.
    - O display e os LEDs fornecem feedback sobre o sucesso ou falha do envio. O LED verde acende em caso de sucesso.

---

## 🚀 Como Executar

1.  **Pré-requisitos:**

    - Visual Studio Code com a extensão PlatformIO IDE.
    - Hardware listado na seção Hardware Utilizado.
    - O Backend InfraMed deve estar em execução na rede local.

2.  **Clone o repositório:**

    ```bash
    git clone <URL_DO_SEU_REPOSITORIO_IOT>
    cd <NOME_DA_PASTA_DO_PROJETO>
    ```

3.  **Abra o projeto no VS Code com PlatformIO.**

4.  **Configure o ambiente:**

    - Abra o arquivo `src/main.cpp`.
    - Ajuste as variáveis de configuração de rede e do servidor:

    ```cpp
    // --- Configurações Wi-Fi ---
    const char *ssid = "Senai-IoT";
    const char *password = "senaiiot";

    // --- Configurações do Servidor HTTP ---
    // Altere para o IP e porta do seu servidor backend
    const char *serverBaseUrl = "http://192.168.1.14:8080/api/leituras/atendimento/";
    ```

5.  **Calibre a balança (Opcional, mas recomendado):**

    - O fator de calibração da balança (`scale.set_scale()`) é específico para cada célula de carga. Utilize um sketch de calibração para encontrar o valor correto para o seu hardware.

6.  **Compile e envie para o ESP32:**

    - Conecte o ESP32 ao seu computador.
    - Use os comandos do PlatformIO para compilar (`Build`) e enviar (`Upload`) o código para o dispositivo.

7.  **Utilize o dispositivo:**
    - Siga o fluxo de funcionamento descrito para registrar uma nova leitura de medicação.

---

## 💡 Contexto do Projeto

Este projeto foi desenvolvido como Trabalho de Conclusão de Curso (TCC) do curso Técnico de Desenvolvimento de Sistemas da Escola SENAI 403 "Antônio Ermírio de Moraes". O objetivo foi aplicar conceitos de sistemas embarcados, Internet das Coisas (IoT) e integração de sistemas para criar uma solução prática e relevante para o setor de saúde, complementando o sistema backend InfraMed.

---

## ✍️ Autor

**Matheus Nunes da Silva**

- **GitHub:** https://github.com/matheus05dev

---
