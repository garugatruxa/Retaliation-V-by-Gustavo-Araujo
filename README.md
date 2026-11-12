# Retaliation V — by Gustavo Araújo

Mod para **GTA V Enhanced (PC)**, só no **modo história**, que faz o submundo de Los Santos e Blaine County
**reagir aos seus crimes**.

Quando a polícia não consegue te pegar, o submundo assume. Cada NPC que você mata, cada carro que você rouba
ou explode aumenta a **pressão de retaliação**. Quanto maior a pressão, mais pesados ficam os grupos que vêm
te caçar: gangues de rua, caipiras, The Lost MC, Triads, cartel, a segurança de Cayo Perico, caçadores de
recompensa, os mercenários russos do Bogdan e, no limite, a **Merryweather**, com helicópteros armados,
Juggernauts, tanques, caças e paraquedistas.

É a continuação, para o GTA V, do
[Retaliation Mod do GTA San Andreas](https://github.com/garugatruxa/Retaliation-Mod-by-Gustavo-Araujo).

> **Download:** baixe a versão mais recente na aba [**Releases**](../../releases). Cada versão tem um `.zip`
> com o `Retaliation.asi` pronto. Todas as versões continuam disponíveis, para quem quiser ver como o mod
> evoluiu ou preferir uma versão mais simples.

---

## Índice

- [Como funciona](#como-funciona)
  - [Menu (Insert ou controle)](#menu-insert-ou-controle)
  - [Modos e o ciclo](#modos-e-o-ciclo)
  - [Pontos de retaliação](#pontos-de-retaliação)
  - [Níveis e facções](#níveis-e-facções)
  - [Waves](#waves)
  - [Combate e IA](#combate-e-ia)
  - [Arsenal e equipamento](#arsenal-e-equipamento)
  - [Helicópteros](#helicópteros)
  - [Juggernaut](#juggernaut)
  - [Tanques e caça](#tanques-e-caça)
  - [Evento: Titan com paraquedistas](#evento-titan-com-paraquedistas)
  - [Minimapa e barra](#minimapa-e-barra)
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

### Menu (Insert ou controle)

> **O mod vem desligado.** Na primeira vez, abra o menu e mude a linha **Mod** para **Ativado**.

| | Teclado | Controle (Xbox / PlayStation) |
|---|---|---|
| Abrir / fechar | **Insert** | **RB + ←** (R1 + ← no PlayStation) |
| Escolher a linha | **↑ / ↓** | direcional **↑ / ↓** |
| Mudar o valor | **← / →** | direcional **← / →** |
| Fechar | **Insert** | **B** (Círculo) |

Com o menu aberto, as setas e o direcional não mexem no celular nem no jogo. As escolhas ficam salvas no
`Retaliation.ini`. O menu segue o **idioma do jogo**: português, espanhol ou, nos outros idiomas, inglês.

| Opção | Valores |
|---|---|
| **Mod** | Ativado / Desativado |
| **Modo** | Junto com a polícia / Substitui a polícia / Ciclo / Desordem |
| **Trava de nível** | Ligada / Desligada |
| **Nível** | Nenhum, 1 a 8 ou **Máximo** (barra cheia) |

- **Nível:** muda a pressão direto pelo menu. **Máximo** enche a barra (500 pontos) e deixa as chances dos
  especiais no topo.
- **Trava de nível:** com ela ligada, o nível não sobe nem desce sozinho. Crimes não dão pontos, a pressão não
  cai com o tempo, o timer não derruba e morrer não zera. Só o menu e as teclas de teste mudam o nível.

Desligar o mod ou trocar de modo remove os retaliadores, zera a pressão e devolve a polícia ao normal.

### Modos e o ciclo

- **Junto com a polícia** (padrão): os crimes dão pontos sempre, e a polícia age normalmente ao mesmo tempo.
  Polícia e retaliadores ficam do mesmo lado: não se atacam e vêm todos atrás de você.
- **Substitui a polícia:** você nunca ganha estrelas. Quem vem atrás de você são os retaliadores.
- **Ciclo:** alterna entre a polícia e o Retaliation.
  1. **Livre.** Só a polícia age, normalmente. Fora do ciclo, os crimes não dão pontos.
  2. **Polícia.** Com **5 estrelas**, sobreviva **5 minutos** sem morrer nem ser preso.
  3. **Retaliation.** A polícia vai embora e o Retaliation começa no **nível 2**. A polícia não volta até a
     retaliação acabar.
  4. **Fim.** A retaliação acaba quando você despista (a pressão cai até zerar) ou **sobrevive aos 5 minutos
     acima de 350 pontos**. Morrer ou ser preso também zera. Livre, o ciclo recomeça.
- **Desordem:** a polícia e os retaliadores entram em guerra, com você no meio.
  - **A polícia é do mod.** A polícia do jogo fica desligada (sem estrelas, sem viaturas do jogo). No lugar dela
    vem a **polícia da barra azul**, com as viaturas, uniformes, armas e acessórios da própria polícia do jogo.
  - **Duas barras:** a **vermelha** (retaliação) sobe um pouco e a **azul** (polícia) fica logo abaixo.
  - Começa no **nível 3** nas duas barras, ou no nível escolhido no menu. No **primeiro minuto** nada baixa;
    depois, as duas baixam normalmente.
  - **Ponto de encontro: você.** A polícia e os retaliadores vêm para a sua área. A briga só
    acontece numa **zona de 50 m em volta de você**: quem está mais longe vem andando até ela sem brigar no
    caminho. Dentro da zona, cada retaliador vai atrás do **policial mais perto** dele, e cada policial vai atrás
    do **retaliador mais perto**, trocando de alvo quando o dele cai ou aparece outro mais perto.
  - A polícia e os retaliadores **brigam entre si** e **só atacam você se você atacar primeiro**, mesmo que você
    passe colado neles: ferir um policial ou uma viatura põe a polícia contra você por 60 s, e ferir um
    retaliador ou um veículo dele põe os retaliadores contra você por 60 s.
  - **Qualquer morte ou destruição** a até 200 m de você, sua ou não, sobe **as duas barras igualmente**
    (+2 por pessoa, +4 por veículo; as suas mortes e explosões valem os pontos normais, também nas duas).
  - **A força da polícia segue a barra azul**:

    | Nível da polícia | Quem vem |
    |---|---|
    | 1 a 4 | Patrulha: LSPD na cidade, xerife no interior (pistola, escopeta, a partir do nível 3 a **submetralhadora da NOOSE** e, mais alto, carabina) |
    | 3 a 6 | **Heavy da polícia**: 1.000 de vida, colete pesado e arma de cadência alta com carregador estendido |
    | 5 | **SWAT** na van blindada |
    | 6 | **FIB**, o **helicóptero da polícia** e o **Juggernaut da polícia** (traje pesado preto com a máscara azul e preta) |
    | 7 | **IAA** |
    | 8 | **Exército**: fuzileiros no Crusader ou no Barracks, **tanque Rhino** e **caça Lazer** (um de cada por vez) |

  - Os policiais aparecem no minimapa com **blips azuis**, iguais aos dos retaliadores.
  - Os retaliadores **também atacam civis**, e 15% dos civis, a pé ou dirigindo, andam com **arma curta**
    (pistola, Micro SMG, Mini SMG ou Machine Pistol). Os **civis armados revidam**: não fogem, enfrentam a
    polícia e os retaliadores, e os dois lados atacam eles também. Com você, eles só brigam se você atacar.
  - Os carros dos retaliadores vêm até a sua área e a tropa desce para lutar.
  - Os retaliadores também trazem os **tanques e o caça** deles, que miram a polícia e o Exército (nunca você, a não
    ser que você ataque os retaliadores). Dos helicópteros com mísseis ou canhão explosivo, só vem o **Buzzard**,
    até o Exército chegar (polícia no nível 8). Aviões perseguidores não vêm.
  - Morrer ou ser preso faz a Desordem recomeçar no nível 3, 10 s depois de você voltar.

Os pontos são **separados por personagem** (Michael, Franklin e Trevor). Trocar de personagem remove os
retaliadores, e a pressão de cada um fica guardada. As emboscadas **pausam em missões da história, cutscenes
e interiores**, e durante as missões a polícia funciona normalmente.

### Pontos de retaliação

A pressão vai de **0 a 500**.

| Ação | Pontos |
|---|---|
| Matar qualquer NPC (pedestre, gangue, policial ou retaliador) | +4 |
| Roubar veículo | +4 |
| Explodir veículo | +8 |
| Destruir helicóptero ou avião | +12 |
| 45 s sem crimes | −15 (repete) |
| 5 min acima de 350 pontos | volta ao nível 2 (no Ciclo, encerra a retaliação) |
| Morrer ou ser preso | zera |

- **Escala decrescente:** a partir de 250 pontos, cada 25 pontos acima tiram **0,25** de cada ganho, com
  mínimo de 0,5 por ação. As frações se acumulam. Chegar a 500 é possível, mas demora.
- NPCs de missão não contam. Os veículos e aeronaves dos retaliadores contam.

### Níveis e facções

Do nível 1 ao 6, a facção depende da região: **cidade** (Los Santos) ou **interior** (Blaine County: Sandy
Shores, Grapeseed, Paleto, deserto e montanhas). Cada facção fica bastante tempo em campo antes da próxima.

| Nível | Pontos | Cidade | Interior |
|---|---|---|---|
| 1 | 10–79 | Marabunta Grande | Caipiras de Sandy Shores |
| 2 | 80–139 | Vagos | Irmãos O'Neil |
| 3 | 140–199 | Ballas | Altruístas |
| 4 | 200–259 | Kkangpae | The Lost MC |
| 5 | 260–319 | Triads (Wei Cheng) | Cartel Madrazo |
| 6 | 320–379 | Segurança de Cayo Perico (El Rubio) | Caçadores de recompensa |
| 7 | 380–439 | Mercenários russos do Bogdan | Mercenários russos do Bogdan |
| 8 | 440–500 | Merryweather | Merryweather |

- A cada nível aumentam a quantidade de inimigos, a mira, a vida, o colete e a frequência das waves.
- Nenhuma facção é do governo: não há FIB, IAA nem militares. As facções não brigam entre si. Com a polícia,
  só brigam no modo **Desordem**; nos outros modos, polícia e retaliadores ficam do mesmo lado, contra você.
- Cada facção tem os seus veículos: lowriders e personalizados nas gangues, off-road no interior, motos custom
  no Lost MC, SUVs blindados no cartel, e blindados na elite (Insurgent, Kuruma blindado, Nightshark, Duke
  O'Death, Ardent, Dune FAV, Technical). Nada de veículos militares de Fort Zancudo.
- **Caçadores, Bogdan e Merryweather** lutam no estilo da NOOSE: média distância, cobertura, flanqueio e
  **sinais de mão**. As gangues partem para cima.

### Waves

| Onde você está | O que vem |
|---|---|
| **A pé** | Grupos a pé, fora do campo de visão, e veículos de longe. Bicicletas nos níveis 1–2. |
| **No mato, montanha ou campo** (a mais de 60 m de uma estrada) | Os grupos a pé nascem **numa estrada longe** (até 650 m) e vêm correndo. Ninguém aparece em volta de você. |
| **De carro devagar** | Só veículos. A partir do nível 3, **bloqueios de estrada** à frente, na sua mão da pista. |
| **De carro rápido** (acima de 65 km/h) | **Perseguição:** carros nascem atrás de você, na sua mão, já na sua velocidade, e os bloqueios ficam bem mais frequentes. |
| **De barco** | Lanchas e jet skis das facções que têm (nível 4+) e helicópteros. |
| **Em mar aberto** (de barco, jet ski ou nadando, sem terra a 110 m) | Só **helicópteros de ataque** (o da facção ou um Buzzard, a partir do nível 3, sem rapel) e **barcos inimigos**. |
| **De helicóptero** | Helicópteros (atiradores e armados) e só um veículo por terra. |
| **De avião** (qualquer um) | Só **aviões armados perseguidores**: metralhadora nos níveis baixos (Ultraleve, Seabreeze, Rogue, Starling), mísseis nos altos (Nokota, Molotok, Pyro, Lazer, Hydra, B-11). 1 a 3 ao mesmo tempo. |

- No máximo **30 inimigos ativos** ao mesmo tempo, somando tudo (10 no nível 1).
- Tudo nasce **no chão e no mesmo nível do mapa** que você, e os inimigos **chegam aos poucos**, em fila.
- Inimigos longe demais, ou que não chegam a tempo, são removidos.
- **Civis e trânsito normais:** com estrelas ou retaliação, a cidade continua cheia de pedestres e carros. Com a
  polícia também atrás de você, os retaliadores ficam em 75% do limite.

### Combate e IA

- **Lutas de perto:** cada arma tem um alcance máximo de tiro. Quem está a pé e fora do alcance **corre até
  você** em vez de atirar de longe.

  | Quem | Alcance |
  |---|---|
  | Pistolas, submetralhadoras e escopetas | 35 m |
  | Fuzis, metralhadoras e minigun | 70 m |
  | RPG e lança-granadas | 110 m |
  | Snipers | 130 m |
  | Atiradores em veículos e torretas | 100 m |
  | Helicópteros e aviões | 220 m |

- **Sem visão de você:** se você estiver atrás de uma parede ou objeto, **fora de cobertura**, os inimigos
  procuram um caminho até você e voltam a atirar quando te enxergam. Em cobertura, eles mantêm a posição.
- **Mato e montanha:** longe das estradas, os inimigos a pé ficam agressivos e avançam atirando.
- **Feridos:** a elite sempre arrasta companheiros feridos para a cobertura; nas gangues e no cartel, 60%.
- **Mira "stormtrooper":** tudo que mata rápido erra bastante e espalha os tiros: RPG, lança-granadas,
  tanques, caças, helicópteros, torretas do Valkyrie e a minigun do Juggernaut.

### Arsenal e equipamento

- Quase todas as armas do jogo, incluindo as do GTA Online (Service Carbine, Battle Rifle, Tactical SMG...),
  mas **sem as versões Mk II**. Corpo a corpo (faca, taco e cacetete) só nos níveis 1 e 2.
- **Fuzis e carabinas** a partir do nível 4. **Snipers** a partir do 6. **RPG e lança-granadas** só nos
  níveis 7 e 8, e são raros.
- **Nunca aparecem:** mosquete, Marksman Pistol, Railgun, Up-n-Atomizer, Widowmaker, armas de choque e similares.
- **Acessórios:** as armas podem vir com lanterna, carregador estendido ou de tambor, mira, empunhadura e,
  raramente, supressor. A chance vai de 10% no nível 1 a 80% no nível 8, com até 3 acessórios na mesma arma.
- A partir dos **200 pontos**, cresce a chance de inimigos com **colete, capacete e armas melhores**.

### Helicópteros

A partir do nível 3, cada facção usa os helicópteros que combinam com ela (Frogger, Maverick, SuperVolito,
Annihilator, Valkyrie).

- **Atiradores:** rondam você com a tropa atirando pelas portas.
- **Rapel:** com você a pé, param no ar perto de você e a tropa desce de corda. É o tipo mais comum.
- **Armados** (Buzzard, Savage, Hunter, Akula): só do **nível 7** em diante e raros (Savage, Hunter e Akula
  mais raros que o Buzzard). Vêm só com o piloto, que ataca com as armas do helicóptero em **rajadas**: 3 s
  atirando e 8 s parado. Em mar aberto, aparecem a partir do nível 3.
- O helicóptero de rapel pode **pousar perto de você** para a tropa desembarcar.
- Até **3 helicópteros** ao mesmo tempo, mas o 2º é pouco comum e o 3º é bem raro. Matar o piloto derruba o
  helicóptero.

### Juggernaut

Mini-boss do **nível 6** em diante, com **2.000 de vida**, imune a ataques corpo a corpo (a coronhada não mata ele).
Chega num **Insurgent** blindado e desce perto de você, com **Combat MG**, **escopeta automática** ou, mais
raro, **minigun** (com a rajada bem espalhada). No minimapa, ele aparece como uma **caveira vermelha** grande.
Quando a vida fica baixa e na morte, o corpo cai normalmente (ragdoll).

Ele usa o traje pesado do GTA Online, montado com as peças do próprio jogo: **preto com vermelho**.

| Peça | Opções |
|---|---|
| Traje | Noturno (preto) |
| Máscara | Balística **vermelha e preta** ou balística com óculos vermelhos |
| Capacete | O capacete do traje pesado ou o balístico Merryweather |

No modo Desordem, a polícia tem o seu próprio Juggernaut, com os mesmos números, em **preto com azul** (máscara
balística azul e preta).

### Heavy (Juggernaut leve)

Nos **níveis 3 a 6**, cada facção pode mandar o seu **Heavy**: o chefe da própria facção, com **1.000 de vida**,
colete pesado, máscara ou bandana, chapéu e óculos sorteados, num carro da facção. Ele usa armas de cadência
alta conforme a facção (submetralhadoras, escopeta bullpup, Gusenberg, fuzil) com carregador estendido e aparece
no minimapa com a **caveira das missões de matança do Trevor** (vermelha; a do Heavy da polícia é azul). No
nível 6 vem o Heavy ou o Juggernaut. Com você a pé, os dois também podem chegar num **Annihilator**, que pousa
perto de você e desembarca o Heavy ou o Juggernaut com mais dois.

### Tanques e caça

Só no nível 8, um de cada por vez, cada um com **1 minuto de espera** depois de sair de combate:

- **Rhino no Cargobob:** o Cargobob solta o tanque perto de você. Abater o Cargobob antes da entrega derruba o
  tanque junto. Se o Cargobob perder o motor (PEM) ou baixar demais, o tanque é solto e nunca fica dentro do
  chão.
- **TM-02 Khanjali:** o tanque especial do Online, mais raro (2% por wave, a partir dos 440 pontos). Vem **por
  terra**. As metralhadoras dele erram bastante e todas as armas dele só disparam na janela de tiro.
- O **Rhino** dispara **um tiro a cada 15 segundos**, e só a até 130 m de você.
- Rhino, caça e helicópteros de ataque ficaram mais raros.
- **Tanque roubado:** se você entrar num tanque dos retaliadores, todas as armas ficam liberadas para você.
- **Caça:** um Lazer (ou Hydra, mais raro) alterna canhão e mísseis, ataca por 90 s e vai embora.
- **Armas de veículo em rajadas:** helicópteros armados, aviões, caça e torretas atiram por 3 s e param por 8 s.

### Evento: Titan com paraquedistas

Um evento raro, **do nível 7 em diante**: com **poucos inimigos vivos** (4 ou menos) e você **a pé e no
chão**, há 10% de chance de, no lugar da wave normal, um **Titan** passar bem alto por cima de você e soltar de
4 a 8 inimigos de **paraquedas**. Eles pousam a 20–60 m e partem para o combate. Depois, 3 minutos de espera.

### Minimapa e barra

- Os blips só aparecem quando o inimigo está a até **180 m** (aeronaves a até 450 m). Cada tipo tem o seu
  ícone: **bolinha vermelha** (a pé), **caveira** (Juggernaut), **caveira das matanças do Trevor** (Heavy), **carro**, **moto**, **barco**, **helicóptero com
  a hélice girando**, **jato** e **tanque**.
- A **barra de retaliação** fica embaixo do minimapa, na largura das barras de vida e especial, no mesmo estilo
  delas. Nos últimos 30 s do timer de 5 minutos, ela pisca.
- No modo **Desordem**, a barra vermelha sobe um pouco e a **barra azul** da polícia aparece logo abaixo. Os
  policiais têm os mesmos blips dos retaliadores, só que **azuis**.

### Modo Diretor

No Modo Diretor (Rockstar Editor), o Retaliation funciona com **qualquer NPC** e zera ao entrar ou sair. O
**modo escolhido no menu manda na polícia**, acima da configuração de procurado do Modo Diretor: em "Substitui a
polícia", por exemplo, a polícia não vem mesmo que o Modo Diretor esteja com ela ligada. Para pausar o mod no
Modo Diretor, use `DirectorMode=0`.

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
2. Baixe o `RetaliationV_v11.4.zip` (ou outra versão) na aba [Releases](../../releases) e extraia.
3. **Com o jogo fechado**, copie o **`Retaliation.asi`** para a mesma pasta do jogo.
4. Abra o jogo e entre no modo história. O mod cria o `Retaliation.ini` e o `Retaliation.log` ao lado do `.asi`.
   Ele **vem desligado**: abra o menu (Insert ou RB + ←) e ative.
5. **Atualizando de outra versão:** substitua o `.asi` e apague o `Retaliation.ini` antigo, para o mod criar um
   novo com as opções atuais.

Pasta padrão da Steam: `C:\Program Files (x86)\Steam\steamapps\common\Grand Theft Auto V Enhanced`

> **Usando junto com o Simple Trainer?** Alguns atalhos dele usam as mesmas teclas do Retaliation (Insert,
> PageUp, PageDown e End). Desligue esses atalhos no `trainerV.ini` do trainer, que no Enhanced fica em
> `%appdata%\trainerv\Enhanced\`. Deixe também o "Never Wanted" dele desligado, ou ele briga com os modos de
> polícia do Retaliation.

## Configuração (`Retaliation.ini`)

Todo número do mod fica no `.ini`. Ele **recarrega sozinho** quando você salva o arquivo, com o jogo aberto.

| Seção | O que controla |
|---|---|
| `[General]` | Liga/desliga, modo (1, 2, 3 ou 4), trava de nível, ativo em missões (por sua conta e risco), Modo Diretor, teclas de teste |
| `[Points]` | Pontos de cada crime e decaimento |
| `[Scaling]` | Escala decrescente dos ganhos acima de 250 pontos |
| `[Levels]` | Em que pontos cada nível começa |
| `[HotTimer]` | Timer de 5 min acima de 350 pontos |
| `[Cycle]` | Estrelas e tempo para o Retaliation assumir no Ciclo, e o nível inicial |
| `[Hud]` | Posição, tamanho e cor da barra |
| `[Spawn]` | Distâncias de spawn e remoção, raio dos blips |
| `[Flow]` | Chegada aos poucos: pressão crescente, espaço entre unidades, teto de inimigos por perto |
| `[Level1]`…`[Level8]` | Intervalo das waves, inimigos a pé, veículos, mira, vida, colete e limite de inimigos |
| `[Roadblock]` | Bloqueios de estrada |
| `[Juggernaut]` | Nível mínimo, chance, teto, vida, modelo, ícone, chance e mira da minigun |
| `[Heavy]` | Heavy (Juggernaut leve): níveis, chance, teto e vida |
| `[VehicleGuns]` | Rajadas das armas de veículo: segundos atirando e parado |
| `[Population]` | Civis e trânsito com estrelas, e % do limite de retaliadores com a polícia junto |
| `[Disorder]` | Modo Desordem: polícia do mod (liga/desliga, policiais por nível, limite, nível do helicóptero), nível inicial, minuto sem baixar, zona de combate, tempo contra o jogador, raio e pontos das mortes, % de civis armados, recomeço depois de morrer |
| `[Vehicles]` | Veículos com torreta (Insurgent e Technical) ligados ou não |
| `[WeaponMods]` | Chance de acessórios nas armas |
| `[Paradrop]` | Evento do Titan: nível mínimo, chance, quantos inimigos vivos no máximo e espera |
| `[Heli]` | Chance por nível, 2º e 3º helicóptero, armados (nível mínimo, chance, teto), rapel, pouso e mira |
| `[Tank]` / `[Jet]` | Chance, teto e espera do Rhino, do Khanjali e do caça; intervalo de tiro e altura do tanque no Cargobob |
| `[Weapons]` | Chance de sniper e de RPG/lança-granadas, mira das armas pesadas |
| `[Range]` | Alcance de tiro de cada tipo de arma, de veículos e de aeronaves; chance de arrastar feridos |
| `[Gear]` / `[Escalation]` | A partir de quantos pontos crescem o equipamento e os especiais |

Apague o `.ini` para voltar aos valores padrão.

## Teclas de teste

Com `DebugKeys=1` (padrão), dá para mudar a pressão sem cometer crimes (também dá pelo menu, na linha **Nível**):

- **Subir um nível:** segure **PageDown** e aperte **PageUp**.
- **Descer um nível:** segure **PageUp** e aperte **PageDown**.
- Na **Desordem**, as teclas sobem ou descem **as duas barras** (vermelha e azul) um nível cada.

O `Retaliation.log` registra cada crime, cada nível, cada wave e cada especial, além dos modelos e armas que o
jogo não reconheceu.

---

## Histórico de versões

Todas as versões estão na aba [Releases](../../releases). O código de cada uma fica na tag correspondente
(`v1` a `v11`, com as correções `v6.1`, `v7.1`, `v9.2`, `v9.3`, `v10.1`, `v11.1`, `v11.2`, `v11.3` e `v11.4`).

### v11.4 — atual

Comparado à v11.3:
- **Desordem calibrada:** toda morte ou destruição sobe **as duas barras igualmente** (vermelha e azul). As
  viaturas e aeronaves da polícia destruídas agora também contam.
- Na Desordem, as **teclas de teste** (PageUp/PageDown) sobem ou descem as duas barras juntas, um nível cada.
- A polícia da Desordem usa a **submetralhadora da NOOSE** a partir do nível 3.
- Em todos os modos, o **Heavy** aparece no minimapa com a **caveira das missões de matança do Trevor**: vermelha
  nos retaliadores e azul na polícia. Nova chave `[Heavy] Blip=84`.
- O **Heavy** e o **Juggernaut** (dos retaliadores e da polícia) também podem chegar num **helicóptero
  Annihilator**, com mais dois. Ele **pousa** no chão perto de você (sem rapel), a equipe desce e ele decola e fica
  rondando. Só com você a pé no chão. Novas chaves `[Heavy] HeliChance=30` e `[Juggernaut] HeliChance=30`.

### v11.3

Comparado à v11.2:
- **Desordem com polícia própria:** a polícia do jogo fica desligada e o mod traz a dele, com as viaturas,
  uniformes, armas e acessórios da polícia do jogo. Ela vai até a zona de combate e enfrenta os retaliadores e os
  civis armados; só vai atrás de você se você atacar a polícia.
- **Barra azul da polícia**, embaixo da vermelha (que subiu um pouco). O nível da polícia sobe com as mortes perto
  de você e define quem vem: patrulha (LSPD ou xerife), **Heavy da polícia**, **SWAT**, **FIB**, helicóptero,
  **Juggernaut da polícia**, **IAA** e, no topo, o **Exército** com tanque Rhino e caça Lazer.
- **Blips azuis** para a polícia, iguais aos dos retaliadores.
- **Juggernauts com cores:** o dos retaliadores em preto com vermelho e o da polícia em preto com azul.
- Na Desordem, os **tanques e o caça dos retaliadores voltaram**, para enfrentar o Exército. Os helicópteros com
  mísseis ou canhão explosivo voltam quando o Exército chega.
- Novas chaves em `[Disorder]`: `OwnPolice`, `CopsPerLevel`, `MaxCops` e `CopHeliMinLevel`.

### v11.2

Comparado à v11.1:
- **Desordem com a briga perto de você:** a polícia, os retaliadores e os civis armados só se enfrentam **a até
  50 m de você**. Quem está mais longe vem andando até a zona sem brigar no caminho, e só então escolhe o
  inimigo mais perto. Assim o confronto nunca fica distante. Ajustável em `[Disorder] FightRadius`.
- **Ninguém te ataca por chegar perto:** na Desordem, polícia e retaliadores só revidam se você atacar eles.
  Saiu a regra dos 12 m (`NearDist`).

### v11.1

Comparado à v11:
- **Desordem com alvos de verdade:** cada retaliador vai atrás do **policial (ou civil armado) mais perto** dele, e
  cada policial vai atrás do **retaliador (ou civil armado) mais perto**, trocando de alvo quando o dele morre ou
  aparece outro mais perto. O **ponto de encontro é você**: a polícia é chamada para a sua área e os retaliadores
  vêm para lá, mas a briga não é com você.
- **Civis armados revidam:** na Desordem, eles não fogem, enfrentam a polícia e os retaliadores, e os dois lados
  atacam eles de volta. Os civis desarmados continuam normais, e os armados só atacam você se você atacar eles.
- **Polícia e retaliadores do mesmo lado fora da Desordem:** em **Junto com a polícia**, **Substitui a polícia** e
  **Ciclo**, eles não se atacam mais. É todo mundo contra você.

### v11

Comparado à v10.1:
- **Novo modo Desordem** (modo 4, no menu). A cidade vira uma guerra entre a polícia e os retaliadores, e você
  está no meio:
  - Começa no **nível 3 com 3 estrelas**, ou no nível escolhido no menu (nível N = N estrelas, no máximo 5).
    Nada baixa no **primeiro minuto**; depois, baixa normalmente.
  - **Polícia contra retaliadores:** os dois lados se enfrentam e **só atacam você** se você chegar **muito perto**
    (12 m) ou **atacar primeiro**. Ferir um policial põe a polícia contra você por 60 s; ferir um retaliador põe os
    retaliadores contra você por 60 s.
  - **Qualquer morte ou destruição** perto de você, sua ou não, sobe a retaliação, e as **estrelas acompanham o
    nível** até o máximo.
  - Os retaliadores **também atacam civis**, e **15% dos civis** (a pé ou dirigindo) andam com arma curta:
    pistola, Micro SMG, Mini SMG ou Machine Pistol.
  - **Sem jatos, aviões perseguidores e tanques.** Dos helicópteros com mísseis ou canhão explosivo, só o
    **Buzzard** aparece (Savage, Hunter, Akula e Valkyrie ficam de fora).
  - Morrer ou ser preso faz a Desordem recomeçar no nível 3, 10 s depois de voltar.
- Nova seção `[Disorder]` no `.ini`, com tudo ajustável. **Atualizando:** apague o `Retaliation.ini` antigo.

### v10.1

Correções e ajustes da v10:
- **Armas de veículo travadas de verdade:** na v10, a IA ignorava a trava e o Rhino atirava fora das janelas.
  Agora o **Rhino** dá **1 tiro a cada 15 s**, o **Khanjali** só dispara na janela, e helicópteros armados,
  aviões, caça e torretas atiram em **rajadas** (3 s atirando, 8 s parados). Se você assumir o veículo, as armas
  ficam liberadas.
- **Corpo do Juggernaut:** ele cai normalmente com a vida baixa e na morte, em vez de travar em pé.
- **Juggernaut** a partir do **nível 6**, um pouco mais frequente, com a caveira maior no minimapa.
- **Heavy (Juggernaut leve):** nos níveis 3 a 6, o chefe de cada facção, com 1.000 de vida, colete pesado e
  armas de cadência alta da facção, num carro dela. No nível 6 vem o Heavy ou o Juggernaut.
- **Menos veículos "apelões":** helicópteros de ataque, Rhino, Khanjali e caça mais raros. Savage, Hunter e
  Akula mais raros que o Buzzard.
- **Mar aberto:** de barco, jet ski ou nadando longe da costa, vêm só helicópteros de ataque (a partir do nível
  3, sem rapel) e barcos inimigos.
- **Civis e trânsito normais** com estrelas ou retaliação. Com a polícia junto, os retaliadores ficam em 75% do
  limite.
- Novas seções no `.ini`: `[Heavy]`, `[VehicleGuns]` e `[Population]`. **Atualizando da v10:** apague o
  `Retaliation.ini` antigo para pegar os novos valores de Juggernaut, tanque, caça e helicópteros.

### v10

Comparado à v9.3:
- **Escala de 0 a 500 pontos**, com os níveis bem mais largos (10, 80, 140, 200, 260, 320, 380, 440): cada facção
  fica mais tempo em campo. Ganhos dobrados (NPC +4, veículo +8, aeronave +12), decaimento de −15 a cada 45 s e
  escala decrescente a partir de 250 pontos.
- **Facções novas:** saíram FIB/IAA e os militares (não brigam com a polícia). Entraram a **segurança de Cayo
  Perico** (nível 6, cidade) e os **mercenários russos do Bogdan** (nível 7).
- **Veículos:** saíram os veículos com torreta que expulsam o jogador no modo história (Menacer, Caracara,
  Halftrack, Barrage, Patrol Boat, Dinghy armado) e os veículos militares de Fort Zancudo. Entraram mais
  blindados: Insurgent, Kuruma blindado, Duke O'Death e os veículos do Bunker (Nightshark, Ardent, Dune FAV).
  Torretas que ficaram (Insurgent, Technical): quando você assume uma, ninguém te tira de lá e a tripulação
  inimiga desce.
- **Sem armas Mk II.** As armas podem vir com **acessórios** sorteados por nível.
- **Combate mais próximo:** alcances de tiro menores para todos (incluindo snipers, RPG, veículos e aeronaves),
  e quem está fora do alcance corre até você. Inimigos **procuram você** quando perdem a visão (fora de
  cobertura). No mato e na montanha, ficam agressivos e nascem nas estradas longe.
- **Perseguição na estrada:** correndo de carro, perseguidores nascem atrás de você na sua mão, e os bloqueios
  ficam mais frequentes e sempre na sua pista.
- **Juggernaut:** traje pesado do Online com variações de camuflagem, máscara e capacete; 2.000 de vida; imune a
  corpo a corpo; caveira vermelha no mapa.
- **Blips por tipo** (a pé, Juggernaut, carro, moto, barco, helicóptero, jato e tanque), visíveis a até 180 m.
- **Helicópteros armados** só do nível 7 em diante, raros, só com o piloto usando as armas do helicóptero. Rapel
  mais comum e só com você a pé.
- **Tanques:** tiro a cada 10 s, mira fraca, tanque mais alto no Cargobob, nunca dentro do chão, e o tanque
  roubado atira. Novo tanque especial **Khanjali**, por terra. Espera de 1 minuto entre tanques e entre caças.
- **Modo avião:** pilotando qualquer avião, vêm só aviões armados perseguidores, conforme o nível.
- **Evento raro:** um **Titan** solta paraquedistas (nível 7+, poucos inimigos vivos, você a pé).
- **Limite de 30 inimigos** ativos, somando tudo.
- **Menu:** novas linhas **Trava de nível** e **Nível** (até **Máximo**), Ciclo com nome curto, título centrado e
  **suporte a controle** de Xbox e PlayStation.
- **Ciclo simplificado:** 5 estrelas por 5 minutos, a polícia vai embora e só volta quando a retaliação acaba.
- **Modo Diretor:** o modo do menu manda na polícia, acima da configuração do Modo Diretor.
- **Mira "stormtrooper"** em tudo que mata rápido, com os tiros bem espalhados.
- Inimigos comuns também **arrastam feridos** para a cobertura.
- Saiu a cópia da roupa de Paleto (tecla End).

### v9 (v9.3)

**v9.3:**
- **Armas dos helicópteros:** o Annihilator, o Valkyrie e os helicópteros de ataque usam as metralhadoras, os
  canhões e os mísseis deles. Quem senta numa torreta do helicóptero atira com ela.
- **Rapel sem bug:** o helicóptero só joga as cordas quando está parado no ar e **fica fixo no mesmo lugar** até
  todos descerem. As cordas não esticam mais.
- **Juggernaut:** a armadura de Paleto é guardada por protagonista (Michael, Trevor, Franklin), e cada
  Juggernaut sorteia uma. Durante a missão The Paleto Score, um aviso lembra como copiar.

**v9.2:**
- O mod **vem desligado** e o modo padrão é **Junto com a polícia**. Para começar, aperte Insert e ative.
- **Menu** mais perto do topo da tela, mais compacto, com letras maiores e o título na fonte do GTA (Pricedown).
- O menu e os avisos seguem o **idioma do jogo**: português, espanhol ou, nos outros idiomas, inglês.

Comparado à v8:
- **Chegada aos poucos, como no Retaliation original:**
  - **Fila com espaçamento:** os inimigos saem da fila um de cada vez, com 7–12 s entre um e outro no nível 1 e
    3,5–7 s no nível 8. As waves viram um fluxo contínuo, e não um bolo.
  - **Pressão crescente:** quando o Retaliation começa, o limite de inimigos vivos parte de 4 e sobe até o
    máximo do nível em cerca de 3 minutos.
  - **Teto de inimigos por perto:** no máximo 3 + o nível a menos de 60 m do jogador. O resto espera na fila
    (helicópteros, caça, tanque e bloqueios não entram nessa conta).
  - **Distâncias variadas:** 35% dos grupos a pé nascem mais longe e chegam depois.
- **Funciona longe da cidade:**
  - Spawns mais longe: a pé a 110–170 m e veículos a 320–450 m. A remoção passou para 600 m.
  - No mato e nas montanhas o mod aceita terreno irregular. Se não achar chão por perto, o grupo nasce
    **na estrada mais próxima e vem correndo** até o jogador.
  - **Longe de qualquer estrada:** +30% de chance de helicóptero, e qualquer facção pode mandar um.
  - **Água por perto** (mar, lago, rio): 35% de chance de vir um **barco** na wave, mesmo com o jogador em terra.
- **O tanque só atira no chão:** pendurado no Cargobob, o canhão fica desligado e só libera depois que o
  tanque toca o solo.
- **Escala decrescente de pontos:** a partir de 50 pontos, cada 5 pontos acima tiram 0,2 de cada ganho (matar,
  explodir, roubar), com mínimo de 0,2 por ação. Chegar a 100 ficou bem mais difícil.
- Tudo ajustável nas novas seções `[Flow]` e `[Scaling]` do `.ini`.

### v8

Comparado à v7 (já inclui a correção do tanque da v7.1):
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

### v7 (e v7.1)

**v7.1:** correção. O tanque do Cargobob só atira depois de tocar o chão.

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

### v6 (e v6.1)

**v6.1:** correção. O tanque do Cargobob só atira depois de tocar o chão.

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
- No modo história, o jogo expulsa o jogador da torreta de alguns veículos do Online (Menacer, Halftrack...).
  Por isso eles saíram. O Dune FAV ainda está em observação.
- O **Ctrl+R** do `ScriptHookV.dev` não recarrega o mod no Enhanced: feche o jogo para trocar o `.asi`.
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

Feche o jogo antes de instalar: o `-Install` não copia com o GTA V aberto.

| Arquivo | Conteúdo |
|---|---|
| `src/main.cpp` | Loop principal, personagem, pausas, Modo Diretor, teclas de teste |
| `src/points.cpp` | Detecção de crimes, decaimento, timers e o ciclo polícia → Retaliation |
| `src/factions.cpp` | Facções, veículos, helicópteros, arsenal por nível, relações com a polícia |
| `src/spawner.cpp` | Waves, IA, bloqueios, Juggernaut, helicópteros, tanque e caça |
| `src/hud.cpp` | Barra de retaliação |
| `src/menu.cpp` | Menu (teclado e controle), idioma |
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
