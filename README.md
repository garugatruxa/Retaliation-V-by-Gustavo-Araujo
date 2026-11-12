# Retaliation V — by Gustavo Araújo

Mod para **GTA V Enhanced (PC)**, só no **modo história**, que faz o submundo de Los Santos e Blaine County
**reagir aos seus crimes**.

Quando a polícia não consegue te pegar, o submundo assume. Cada NPC que você mata, cada carro que você rouba
ou explode aumenta a **pressão de retaliação**. Quanto maior a pressão, mais pesados ficam os grupos que vêm
te caçar: gangues de rua, caipiras, The Lost MC, Triads, cartel, agentes federais, caçadores de recompensa e,
no limite, a **Merryweather**, com helicópteros de ataque, Juggernauts, tanque entregue por Cargobob e caça a jato.

É a continuação, para o GTA V, do
[Retaliation Mod do GTA San Andreas](https://github.com/garugatruxa/Retaliation-Mod-by-Gustavo-Araujo).

> **Download:** baixe a versão mais recente na aba [**Releases**](../../releases). Cada versão tem um `.zip`
> com o `Retaliation.asi` pronto. Todas as versões continuam disponíveis, para quem quiser ver como o mod
> evoluiu ou preferir uma versão mais simples.

---

## Índice

- [Como funciona](#como-funciona)
  - [Menu e modos (tecla Insert)](#menu-e-modos-tecla-insert)
  - [O ciclo: polícia → Retaliation → livre](#o-ciclo-polícia--retaliation--livre)
  - [Pontos de retaliação](#pontos-de-retaliação)
  - [Níveis e facções](#níveis-e-facções)
  - [Waves](#waves)
  - [Arsenal e equipamento](#arsenal-e-equipamento)
  - [Helicópteros](#helicópteros)
  - [Juggernaut](#juggernaut)
  - [Tanque no Cargobob e caça a jato](#tanque-no-cargobob-e-caça-a-jato)
  - [Barra de retaliação](#barra-de-retaliação)
  - [Modo Diretor](#modo-diretor)
- [Requisitos](#requisitos)
- [Guia de instalação](#guia-de-instalação)
- [Configuração (`Retaliation.ini`)](#configuração-retaliationini)
- [Teclas de teste](#teclas-de-teste)
- [Histórico de versões](#histórico-de-versões)
- [Problemas conhecidos](#problemas-conhecidos)
- [Compilando a partir do código](#compilando-a-partir-do-código)
- [Créditos](#créditos)
- [Licença](#licença)

---

## Como funciona

### Menu e modos (tecla Insert)

Aperte **Insert** para abrir o menu do mod, no canto esquerdo da tela. Use **↑ / ↓** para escolher a linha e
**← / →** para mudar o valor. Com o menu aberto, as setas não mexem no celular nem no jogo. A escolha fica
salva no `Retaliation.ini`.

| Opção | Valores |
|---|---|
| **Mod** | Ativado / Desativado |
| **Modo** | Junto com a polícia / Substitui a polícia / Ciclo (polícia → Retaliation) |

- **Junto com a polícia** (o original): os crimes dão pontos sempre, e a polícia age normalmente ao mesmo tempo.
  É uma guerra de três lados o tempo todo.
- **Substitui a polícia:** você nunca ganha estrelas. Quem vem atrás de você são os retaliadores.
- **Ciclo** (padrão): explicado logo abaixo.

Desligar o mod ou trocar de modo remove os retaliadores, zera a pressão e devolve a polícia ao normal.

### O ciclo: polícia → Retaliation → livre

1. **Livre.** O Retaliation fica desligado. Quem age é só a polícia, normalmente.
2. **Polícia.** Com **5 estrelas** de procurado, sobreviva **3 minutos** sem morrer nem ser preso.
3. **Fase conjunta (5 minutos).** O Retaliation começa no **nível 2**. Você **continua com as 5 estrelas** e a
   polícia continua vindo, mas passa a **ignorar você**: como se você estivesse invisível, ela só enfrenta os
   retaliadores. É uma guerra de três lados, e você pode atirar em quem quiser.
4. **Retaliation sozinho.** Depois da fase conjunta, as estrelas somem e a polícia fica bloqueada até você ficar
   livre. Agora é você contra o submundo.
5. **A escolha.**
   - **Se esconder:** sem crimes, a pressão cai (−5 a cada 45 s) até zerar, e você fica livre.
   - **Continuar o caos:** os pontos sobem e os grupos ficam cada vez mais pesados.
   - **Sobreviver 5 minutos acima de 70 pontos** faz a pressão voltar ao **nível 2**, e você escolhe de novo.
6. **Morrer ou ser preso** zera tudo. Livre, o ciclo recomeça.

Os pontos são **separados por personagem** (Michael, Franklin e Trevor). Trocar de personagem remove os
retaliadores, e a pressão de cada um fica guardada. As emboscadas **pausam em missões da história, cutscenes
e interiores**, e durante as missões a polícia funciona normalmente.

### Pontos de retaliação

| Ação | Pontos |
|---|---|
| Matar qualquer NPC (pedestre, gangue, policial ou retaliador) | +2 |
| Roubar veículo | +2 |
| Explodir veículo | +4 |
| Destruir helicóptero ou avião | +6 |
| 45 s sem crimes | −5 (repete) |
| 5 min acima de 70 pontos | volta ao nível 2 |
| Morrer ou ser preso | zera |

- O máximo é 100. Fora do ciclo, os crimes não dão pontos.
- NPCs de missão não contam. Os veículos e aeronaves dos retaliadores contam.

### Níveis e facções

Do nível 1 ao 5, a facção depende da região: **cidade** (Los Santos) ou **interior** (Blaine County: Sandy
Shores, Grapeseed, Paleto, deserto e montanhas). Cada facção aparece em um único nível.

| Nível | Pontos | Cidade | Interior |
|---|---|---|---|
| 1 | 10–20 | Marabunta Grande | Caipiras de Sandy Shores |
| 2 | 21–31 | Vagos | Irmãos O'Neil |
| 3 | 32–42 | Ballas | Altruístas |
| 4 | 43–53 | Kkangpae | The Lost MC |
| 5 | 54–64 | Triads (Wei Cheng) | Cartel Madrazo |
| 6 | 65–75 | Agentes FIB / IAA | Agentes FIB / IAA |
| 7 | 76–87 | Caçadores de recompensa | Caçadores de recompensa |
| 8 | 88–100 | Merryweather | Merryweather |

- A cada nível aumentam a quantidade de inimigos, a mira, a vida, o colete e a frequência das waves.
- Cada facção tem os seus veículos: lowriders e personalizados nas gangues, off-road no interior, motos custom
  no Lost MC, SUVs blindados no cartel e na FIB, e blindados com torreta na elite (Technical, Caracara,
  Menacer, Halftrack, Insurgent). Quando o veículo tem torreta, um atirador vem nela.
- As facções não brigam entre si, só com você e com a polícia. A FIB é aliada da polícia.
- **Merryweather e Caçadores** lutam no estilo da NOOSE: ficam a média distância, usam cobertura, flanqueiam e
  trocam **sinais de mão**. As gangues partem para cima.

### Waves

| Onde você está | O que vem |
|---|---|
| **A pé** | Duplas a pé a média distância, fora do campo de visão, e veículos de longe. Bicicletas nos níveis 1–2. |
| **De carro ou moto** | Só veículos. A partir do nível 3, **bloqueios de estrada** à frente: carros atravessados e atiradores atrás. |
| **De barco** | Lanchas e jet skis das facções que têm (nível 4+) e helicópteros. |
| **Voando** | Foco em helicópteros (atiradores e ataque, nunca rapel), o caça no nível 8 e só um veículo por terra. |

- Até **28 inimigos** ao mesmo tempo no nível máximo (10 no nível 1).
- Tudo nasce **no chão e no mesmo nível do mapa** que você: ninguém aparece em cima de um viaduto quando você
  está embaixo, nem cai do céu.
- Inimigos longe demais, ou que não chegam a tempo, são removidos.
- Os **blips vermelhos** só aparecem no minimapa quando o inimigo está a até 120 m.

### Arsenal e equipamento

- Quase todas as armas do jogo, incluindo as do GTA Online (Service Carbine, Battle Rifle, Tactical SMG,
  armas Mk II...). Corpo a corpo (faca, taco e cacetete) só nos níveis 1 e 2.
- **Fuzis e carabinas** a partir do nível 4. **Snipers** a partir do 6. **RPG e lança-granadas** só nos
  níveis 7 e 8, e são raros.
- **Nunca aparecem:** mosquete, Marksman Pistol, Railgun, Up-n-Atomizer, Widowmaker, armas de choque e similares.
- **Alcance por arma:** pistolas, submetralhadoras e escopetas só atiram de perto (45 m). Fuzis e metralhadoras
  atiram mais longe (110 m). Só snipers e RPG atiram assim que te veem.
- A partir dos **40 pontos**, cresce a chance de inimigos com **colete, capacete e armas melhores**.
- **Mira cinematográfica:** quem usa explosivos (RPG, lança-granadas, tanque, caça e helicóptero de ataque) erra
  de propósito. Os tiros passam perto e explodem em volta.

### Helicópteros

A partir do nível 3, cada facção usa os helicópteros que combinam com ela (Frogger, Maverick, SuperVolito,
Annihilator, Valkyrie).

- **Atiradores:** rondam você com a tropa atirando pelas portas.
- **Rapel:** param perto de você e a tropa desce de corda.
- **Ataque:** Buzzard, e na Merryweather também Savage e Hunter. São mais raros.
- Helicópteros de ataque e de rapel podem decidir **pousar perto de você** (se você estiver a pé). A tropa
  desembarca e vai atrás de você.
- Até **3 helicópteros** ao mesmo tempo, mas o 2º é pouco comum e o 3º é bem raro.
- Matar o piloto derruba o helicóptero.

### Juggernaut

Mini-boss dos níveis 7 e 8, com a armadura pesada do assalto de Paleto Bay. Chega **sentado na caçamba de um
Insurgent**, com motorista e um atirador na torreta .50, e desce perto de você. Vem com **Combat MG** ou com a
**escopeta automática de tambor**. A **minigun** é rara.

O Juggernaut é um **humano com a armadura pesada do modo história** (a da missão The Paleto Score), e não o
robô do Online. Essa armadura só existe nas roupas dos protagonistas, então o mod copia a roupa direto do jogo:

1. Repita a missão **The Paleto Score** pelo replay de missões.
2. Quando o personagem estiver com a armadura pesada, **segure PageDown e aperte End**.
3. O mod salva a roupa no `Retaliation.ini`, em `[JuggernautOutfit]`, e os próximos Juggernauts já vêm com ela.

Até a roupa ser copiada, o Juggernaut é um mercenário da Merryweather com capacete e colete pesado.

### Tanque no Cargobob e caça a jato

Só no nível 8, um de cada por vez:

- **Tanque:** um Cargobob chega com o Rhino pendurado no gancho e solta o tanque perto de você. Se você
  **abater o Cargobob antes da entrega**, o tanque cai junto.
- **Caça:** um Lazer (ou Hydra, mais raro) alterna canhão e mísseis, ataca por 90 s e vai embora.

As chances dos especiais crescem a partir dos 70 pontos, sempre com um teto.

### Barra de retaliação

Uma barra vermelha embaixo do minimapa, na largura das barras de vida, fôlego e especial, no mesmo estilo
delas. O fundo é escuro, o preenchimento é forte, e o trecho novo **brilha** quando a barra enche. Nos
últimos 30 s do timer de 5 minutos, ela pisca.

### Modo Diretor

No Modo Diretor (Rockstar Editor), o Retaliation funciona com **qualquer NPC**. Ao entrar ou sair, ele zera.
Lá dentro, os pontos sobem direto pelos crimes ou pelas [teclas de teste](#teclas-de-teste), sem o ciclo da
polícia. Para pausar o mod no Modo Diretor, use `DirectorMode=0`.

---

## Requisitos

- **GTA V Enhanced** (PC, Steam, Epic ou Rockstar). Só modo história.
- [**Script Hook V**](http://www.dev-c.com/gtav/scripthookv/) de Alexander Blade, na versão compatível com o
  seu jogo. Ele traz o carregador de `.asi`.

> O Script Hook V fecha o jogo se você entrar no **GTA Online**. Use o mod só no modo história.

## Guia de instalação

1. Baixe o **Script Hook V** no [site oficial](http://www.dev-c.com/gtav/scripthookv/) e copie o conteúdo da
   pasta `bin` (menos o `NativeTrainer.asi`, se não quiser) para a pasta do jogo, onde fica o `GTA5_Enhanced.exe`.
   Siga o `HOW_TO_INSTALL` que vem com ele.
2. Baixe o `RetaliationV_v8.zip` (ou outra versão) na aba [Releases](../../releases) e extraia.
3. Copie o **`Retaliation.asi`** para a mesma pasta do jogo.
4. Abra o jogo e entre no modo história. O mod cria o `Retaliation.ini` e o `Retaliation.log` ao lado do `.asi`.
5. **Atualizando de outra versão:** substitua o `.asi` e apague o `Retaliation.ini` antigo, para o mod criar um
   novo com as opções atuais.

Pasta padrão da Steam: `C:\Program Files (x86)\Steam\steamapps\common\Grand Theft Auto V Enhanced`

## Configuração (`Retaliation.ini`)

Todo número do mod fica no `.ini`. Ele **recarrega sozinho** quando você salva o arquivo, com o jogo aberto.

| Seção | O que controla |
|---|---|
| `[General]` | Liga/desliga, modo (1, 2 ou 3, também pelo menu), ativo em missões (por sua conta e risco), Modo Diretor, teclas de teste |
| `[Points]` | Pontos de cada crime e decaimento |
| `[Levels]` | Em que pontos cada nível começa |
| `[HotTimer]` | Timer de 5 min acima de 70 pontos |
| `[Cycle]` | Estrelas e tempo para o Retaliation assumir, nível inicial e duração da fase conjunta |
| `[Hud]` | Posição, tamanho e cor da barra |
| `[Spawn]` | Distâncias de spawn e remoção, raio dos blips |
| `[Level1]`…`[Level8]` | Intervalo das waves, inimigos a pé, veículos, mira, vida, colete e limite de inimigos |
| `[Roadblock]` | Bloqueios de estrada |
| `[Juggernaut]` | Chance, teto, vida e chance de minigun |
| `[Heli]` | Chance por nível, 2º e 3º helicóptero, ataque, rapel e pouso |
| `[Tank]` / `[Jet]` | Chance e teto do tanque e do caça |
| `[Weapons]` | Chance de sniper e de RPG/lança-granadas, mira dos explosivos |
| `[Range]` | Alcance de tiro das armas curtas e médias |
| `[Gear]` / `[Escalation]` | A partir de quantos pontos crescem o equipamento e os especiais |

Apague o `.ini` para voltar aos valores padrão.

## Teclas de teste

Com `DebugKeys=1` (padrão), dá para mudar a pressão sem cometer crimes:

- **Subir um nível:** segure **PageDown** e aperte **PageUp**.
- **Descer um nível:** segure **PageUp** e aperte **PageDown**.
- **Copiar a roupa atual para o Juggernaut:** segure **PageDown** e aperte **End**.

O `Retaliation.log` registra cada crime, cada nível, cada wave e cada especial, além dos modelos e armas que o
jogo não reconheceu.

---

## Histórico de versões

Todas as versões estão na aba [Releases](../../releases). O código de cada uma fica na tag correspondente
(`v1` a `v8`).

### v8 — atual

Comparado à v7:
- **Menu do mod na tecla Insert**, no canto esquerdo, com texto branco e detalhes no vermelho da barra. As setas
  escolhem e mudam as opções, e a escolha fica salva no `.ini`.
- **Mod ativado/desativado** direto pelo menu, sem precisar mexer no `.ini`.
- **Três modos** à escolha, que juntam as formas que o mod já teve:
  - **Junto com a polícia:** o original (v2–v6). Os crimes sempre dão pontos e a polícia age ao mesmo tempo.
  - **Substitui a polícia:** sem estrelas, só os retaliadores.
  - **Ciclo:** o da v7, com polícia, fase conjunta, Retaliation e livre.
- Trocar de modo ou desligar o mod limpa tudo e devolve a polícia ao normal.
- **Juggernaut humano:** a armadura pesada do assalto de Paleto Bay no lugar do robô do Online. A roupa é
  copiada do jogo (segure PageDown e aperte End com o personagem de armadura).

### v7

Comparado à v6:

**O ciclo polícia → Retaliation → livre**
- O Retaliation fica desligado até a polícia desistir. Fora dele, os crimes não dão pontos.
- **5 estrelas por 3 minutos** sem morrer nem ser preso: o Retaliation começa no **nível 2**.
- **Fase conjunta (5 min):** as estrelas continuam e a polícia segue vindo, mas **ignora o jogador** e só
  enfrenta os retaliadores.
- Depois disso as estrelas somem e a polícia fica bloqueada até o jogador ficar livre. Nas missões da história
  ela funciona normalmente.
- **5 min acima de 70 pontos** fazem voltar ao **nível 2**. Para ficar livre, é preciso se esconder até zerar.
- Substitui a regra da v4 ("5 estrelas por 5 min caem para 2").

**Pontos**
- **Qualquer NPC morto vale +2**: pedestre, gangue, policial e agora também os **retaliadores**. Os veículos e
  aeronaves deles também contam.

**Outros**
- **Teclas de teste sem Ctrl:** segure PageDown e aperte PageUp para subir um nível, ou o inverso para descer.
- **Barra na largura inteira** do minimapa, alinhada com as barras de vida, fôlego e especial.
- **Modo Diretor:** o Retaliation funciona com qualquer NPC e zera ao entrar e ao sair.
- Versão única, sem presets de desempenho: como a polícia e os mercenários não ficam juntos o tempo todo, há
  menos NPCs ao mesmo tempo.

### v6

Comparado à v5:
- **Armas do GTA Online:** SNS Pistol Mk II, WM 29, AP Pistol, Tactical SMG, Revolver Mk II, Perico Pistol,
  Service Carbine e Battle Rifle.
- **Veículos do Online "pé no chão":** lowriders e personalizados da Benny's para Marabunta, Vagos e Ballas;
  off-road (Kalahari, Riata, Yosemite, Kamacho) para os caipiras; motos custom para o Lost MC; SUVs blindados
  para o cartel e a FIB; Technical, Caracara, Menacer e Nightshark para os Caçadores; Halftrack, Barrage e
  Insurgent armado para a Merryweather.
- **Torretas tripuladas:** nos veículos com torreta, vem um atirador nela.
- **Proibidos:** veículos com nitro, pulo, mísseis ou que voam (Oppressor, Toreador, Ruiner 2000, Scramjet,
  Vigilante, Deluxo...) e armas como Up-n-Atomizer, Widowmaker e armas de choque.
- **Até 3 helicópteros** misturados. Valkyrie e Annihilator Stealth no transporte da Merryweather, Savage e
  Hunter no ataque.
- **Tanque entregue pelo Cargobob** (nível 8, um por vez). Abater o Cargobob derruba o tanque.
- **Caça a jato** (nível 8, um por vez): Lazer ou Hydra, alternando canhão e mísseis.
- **Chances com teto:** 2º helicóptero até 20%, 3º até 8%, tanque até 18%, caça até 15%, Juggernaut até 30%,
  minigun até 25%, RPG/lança-granadas até 20% por wave.
- **Mira imprecisa para explosivos** (RPG, lança-granadas, tanque, caça e helicóptero de ataque).

### v5

Comparado à v4:
- **Alcance de tiro conforme a arma:** pistolas, submetralhadoras e escopetas só de perto; fuzis e metralhadoras
  mais longe; só snipers e RPG atiram assim que veem o jogador.
- **Railgun proibida.**
- **Helicópteros:** atiradores (Frogger/Maverick), rapel (Maverick/Annihilator) e ataque (Buzzard, mais raro).
- Ataque e rapel podem **pousar perto do jogador a pé**, e a tropa desembarca.
- **Jogador voando:** foco em helicópteros (sem rapel) e só um veículo por terra.
- **Juggernaut** com Combat MG ou escopeta automática. A minigun ficou rara.
- A partir dos **70 pontos**, os especiais ficam mais frequentes.

### v4

Comparado à v3:
- Até **28 inimigos** no nível máximo, com o limite subindo por nível.
- Spawns **mais longe**, principalmente os veículos (260–350 m).
- Spawns **no chão e no mesmo nível do mapa** que o jogador (viadutos, pistas sobrepostas).
- **Facções não brigam mais entre si**, só com o jogador e a polícia.
- Saiu o aviso "Retaliação: nível X". A **barra ficou sólida**, sem marcas.
- **Aeronave destruída: +6 pontos.**
- Saiu o "calor máximo". No lugar: **5 min acima de 80 pontos** fazem perder metade dos pontos.
- **5 estrelas por 5 min** fazem o procurado cair para 2 estrelas.

### v3

Comparado à v2:
- **Arsenal muito maior**, com armas Mk II nos níveis altos. Saíram o mosquete e a Marksman Pistol. Corpo a
  corpo só nos níveis 1 e 2 (faca, taco e cacetete).
- A partir dos **40 pontos**: mais colete, capacete e armas melhores.
- **IA no estilo da NOOSE** e **sinais de mão**, só na Merryweather e nos Caçadores.
- Barra mais alta, sem o número do nível.

### v2

Comparado à v1:
- **Os inimigos chegam:** waves a pé e de veículo (com bicicletas nos primeiros níveis), e barcos para quem
  está na água.
- **8 níveis**, com facções diferentes na cidade e no interior.
- **Bloqueios de estrada** à frente de quem está dirigindo.
- **Juggernaut** na caçamba do Insurgent, com minigun.
- **Blips só perto do jogador.**
- Guerra com a polícia e entre as facções.
- Arsenal com mosquete e machete, mantidos de propósito para mostrar a evolução.

### v1 — primeira versão
- Pontos de retaliação por crime, separados por personagem.
- Barra vermelha no estilo da barra do especial, com brilho ao encher.
- Decaimento, morte e prisão zerando, e "calor máximo" (2 min com 95+ pontos viram 5 estrelas).
- Pausa em missões, cutscenes e interiores. `.ini` que recarrega sozinho e `Retaliation.log`.
- Ainda **sem inimigos**: é a base do sistema.

---

## Problemas conhecidos

- O mod foi feito e testado no **GTA V Enhanced**. Ele não foi testado na versão Legacy.
- Depois de cada atualização do jogo, é preciso esperar o **Script Hook V** ser atualizado.
- Partes que dependem da física e da IA do jogo podem variar: o tanque no gancho do Cargobob (se o gancho
  falhar, o tanque vai preso direto no helicóptero), os pousos dos helicópteros e a pontaria do caça.
- Alguns modelos de NPC, veículos ou armas podem não existir em todas as versões do jogo. O mod descarta
  esses modelos sozinho e registra no `Retaliation.log` o que não encontrou.
- `ActiveInMissions=1` é **por sua conta e risco**: os retaliadores podem atrapalhar missões. Salve o jogo antes.

## Compilando a partir do código

O mod é uma DLL `.asi` de 64 bits em C++, compilada com o [llvm-mingw](https://github.com/mstorsjo/llvm-mingw).

1. Baixe o **SDK do Script Hook V** no [site oficial](http://www.dev-c.com/gtav/scripthookv/). Os arquivos dele
   não podem ser redistribuídos, por isso não estão no repositório.
2. Copie `natives.h`, `nativeCaller.h`, `types.h` e `enums.h` da pasta `inc` do SDK para `sdk\inc\`. **Não**
   copie o `main.h`: o repositório usa o próprio `sdk\inc\main.h`, que carrega o `ScriptHookV.dll` em tempo de
   execução, porque o `.lib` do SDK só funciona com o compilador da Microsoft.
3. No PowerShell, na pasta do projeto:

```powershell
.\build.ps1            # gera build\Retaliation.asi
.\build.ps1 -Install   # compila e copia para a pasta do jogo
```

Com o arquivo vazio `ScriptHookV.dev` na pasta do jogo, **Ctrl+R** recarrega o mod sem fechar o jogo.

| Arquivo | Conteúdo |
|---|---|
| `src/main.cpp` | Loop principal, personagem, pausas, Modo Diretor, teclas de teste |
| `src/points.cpp` | Detecção de crimes, decaimento, timers e o ciclo polícia → Retaliation |
| `src/factions.cpp` | Facções, veículos, helicópteros, arsenal por nível, relações com a polícia |
| `src/spawner.cpp` | Waves, IA, bloqueios, Juggernaut, helicópteros, tanque e caça |
| `src/hud.cpp` | Barra de retaliação |
| `src/config.cpp` | `Retaliation.ini` e `Retaliation.log` |

## Créditos

- **Ideia, design e testes:** Gustavo Araújo
- **Desenvolvimento:** feito com o auxílio do [Claude Code](https://claude.com/claude-code) (Claude, da Anthropic)
- Requer o [Script Hook V](http://www.dev-c.com/gtav/scripthookv/), de Alexander Blade
- Inspirado no [Retaliation Mod para GTA San Andreas](https://github.com/garugatruxa/Retaliation-Mod-by-Gustavo-Araujo),
  do mesmo autor

## Licença

Distribuído sob a licença [MIT](LICENSE): qualquer pessoa pode usar, modificar e redistribuir o mod, desde que
mantenha o aviso de copyright.

GTA V é uma marca da Rockstar Games. Este é um mod de fã, sem vínculo com a Rockstar.
