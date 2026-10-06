# OmniAqua — Embedded System for Automated Balling Dosing

Repositório oficial do firmware e da documentação de engenharia do **OmniAqua**, um sistema embarcado autônomo voltado para a dosagem precisa e controlada de macroelementos em aquários de recife (Método Balling).

---

## 📌 Visão Geral do Sistema
O ecossistema de um aquário marinho de recife exige a reposição contínua e rigorosa de Cálcio ($Ca$), Alcalinidade ($KH$) e Magnésio ($Mg$) para evitar a desestabilização dos parâmetros físico-químicos e a degradação de organismos calcificadores. A dosagem manual apresenta alta variabilidade e riscos críticos, como a precipitação de carbonato de cálcio ($CaCO_{3}$) decorrente do contato direto entre soluções de cálcio e carbonatos em alta concentração.

O **OmniAqua** mitiga essas falhas operacionais através de uma arquitetura baseada em microcontrolador, agendamento temporal via rede e controle estrito de atuadores de potência.

---

## 🛠️ Especificações Técnicas de Hardware

### 1. Unidade de Processamento e Conectividade
* **Microcontrolador:** ESP8266 NodeMCU (SoC Tensilica Xtensa 32-bit LX106, 80MHz, Wi-Fi 802.11 b/g/n nativo).
* **Sincronização Temporal:** Protocolo NTP (*Network Time Protocol*) via Wi-Fi, eliminando a necessidade de um circuito de relógio de tempo real (RTC) externo.

### 2. Atuadores e Mecatrônica
* **Bombas Peristálticas:** 3x Kamoer NKP-DC-S04B (12V DC, corrente nominal $\le 0.3\text{A}$, vazão calibrada $\approx 10\text{ mL/min}$).
* **Isolamento Volumétrico:** Linhas de fluidos estritamente independentes para cada solução ($Ca$, $KH$, $Mg$).

### 3. Eletrônica de Potência e Circuitos de Acionamento (*Low-Side Switching*)
O circuito opera com **dois domínios elétricos isolados** que compartilham exclusivamente um terra comum (GND):
* **Domínio de Potência (12V):** Alimentação direta das cargas indutivas (motores das bombas).
* **Domínio Lógico (3.3V/5V):** Alimentação do ESP8266 via conversor Buck DC-DC step-down (eficiência térmica superior a reguladores lineares).

Para cada um dos 3 canais de acionamento, a interface de potência é composta por:
* **Transistor MOSFET (Canal N - Nível Lógico):** Modelo `IRLZ44N` (permite saturação completa em $V_{gs} = 3.3\text{V}$, atuando como chave eletrônica rápida).
* **Resistor de Gate ($R_g$):** $220\,\Omega$ (limitação de corrente transiente no gate do MOSFET).
* **Resistor Pull-down ($R_{pd}$):** $10\text{ k}\Omega$ (garante estado logicamente baixo no gate durante o *boot* e estados *floating* dos GPIOs do ESP8266).
* **Diodo Flyback / Roda Livre:** Diodo Schottky `1N5822` (3A/40V) em arranjo anti-paralelo com o motor para supressão de surtos induzidos por colapso de campo magnético ($V = -L \cdot di/dt$).

