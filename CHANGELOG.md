# Retaliation V — histórico de versões

Cada versão fica publicada separadamente, para mostrar a evolução do mod. As builds estão em `releases\`.

## v1 — a base
- Pontos de retaliação por crime: civil +4, polícia +5, gangue +2, roubo +2, explosão +4.
- Pontos separados para Michael, Franklin e Trevor. Morrer ou ser preso zera os pontos.
- Decaimento de −5 a cada 45 s sem crime.
- Barra vermelha no estilo da barra do especial, com brilho ao encher.
- Calor máximo: 2 min com 95+ pontos viram 5 estrelas.
- Pausa em missões, cutscenes e interiores. `.ini` que recarrega sozinho e `Retaliation.log`.

## v2 — os inimigos chegam
- Waves: a pé, você recebe duplas a média distância e veículos de longe; de veículo, só vêm veículos; de barco, vêm barcos.
- 8 níveis com facções diferentes na cidade e no interior.
- Bloqueios de estrada à frente de quem está dirigindo.
- Juggernaut na caçamba do Insurgent, com motorista e atirador na torreta.
- Blips vermelhos só perto do jogador. Guerra com a polícia e entre facções.

## v3 — arsenal e táticas
- Arsenal bem mais variado. Sem mosquete e sem Marksman Pistol. Corpo a corpo só nos níveis 1 e 2.
- A partir dos 40 pontos: mais colete, capacete e armas melhores.
- IA no estilo da NOOSE e sinais de mão, só na Merryweather e nos Caçadores.
- Barra mais alta, sem o número do nível.

## v4 — escala e equilíbrio
- Até 28 inimigos no nível máximo. Spawns mais longe.
- Spawns no chão e no mesmo nível do mapa que o jogador (viadutos, pistas sobrepostas).
- Facções não brigam mais entre si.
- Sem o aviso de nível. Barra sólida, sem marcas.
- Aeronave destruída: +6 pontos.
- O calor máximo saiu. No lugar: 5 min com 80+ pontos faz perder metade dos pontos.
- 5 min com 5 estrelas faz o procurado cair para 2 estrelas.

## v5 — helicópteros e alcance
- Alcance de tiro conforme a arma. Só snipers e RPG atiram de longe.
- Railgun proibida.
- Helicópteros: atiradores, rapel e Buzzard de ataque, que é mais raro.
- Ataque e rapel podem pousar perto de quem está a pé, e a tropa desce.
- Com o jogador voando: foco em helicópteros, sem rapel.
- Juggernaut com Combat MG, escopeta automática ou minigun (rara).
- Os especiais ficam mais frequentes a partir dos 70 pontos.

## v6 — arsenal pesado
- Armas e veículos do Online "pé no chão": lowriders e personalizados nas gangues, blindados e com torreta na elite.
- Armas e veículos "apelões" proibidos: nitro, pulo, mísseis, voadores.
- Atiradores tripulam as torretas (Technical, Caracara, Menacer, Halftrack, Insurgent...).
- Até 3 helicópteros misturados. Valkyrie, Savage e Hunter entram na Merryweather.
- Tanque Rhino entregue pelo Cargobob, um por vez.
- Jato Lazer (ou Hydra, mais raro) alternando canhão e mísseis, um por vez.
- Chances com teto para os especiais e as armas perigosas. O 3º helicóptero é bem raro.
- Mira imprecisa de propósito para explosivos (RPG, lança-granadas, tanque, jato e helicóptero de ataque).

## v7 — o ciclo
- O Retaliation fica desligado até a polícia desistir. Fora dele, os crimes não dão pontos.
- 5 estrelas por 3 min sem morrer nem ser preso: o Retaliation começa no nível 2.
- Fase conjunta (5 min): as estrelas continuam e a polícia segue vindo, mas ignora o jogador e só enfrenta os retaliadores.
- Depois da fase conjunta, as estrelas somem e a polícia fica bloqueada até o jogador ficar livre. Nas missões da história ela volta ao normal.
- Qualquer NPC morto vale +2: retaliador, pedestre, gangue ou policial. Os veículos e as aeronaves deles também dão pontos.
- Sobreviver 5 min acima de 70 pontos faz voltar ao nível 2, e aí dá para escolher: voltar ao caos ou se esconder.
- Para ficar livre, é preciso se esconder até o decaimento zerar os pontos.
- Livre, o ciclo recomeça: polícia → Retaliation → livre.
- A barra ocupa a largura inteira do minimapa, alinhada com as barras de vida, fôlego e especial.
- Modo Diretor: ao entrar ou sair, os retaliadores somem e a retaliação zera. Lá dentro ela funciona com qualquer NPC e sobe direto pelos crimes ou pelas teclas de teste, sem o ciclo da polícia. Com `DirectorMode=0`, ela fica pausada.
- Versão única, sem presets de desempenho.
- Teclas de teste sem Ctrl: segure PageDown e aperte PageUp para subir um nível; segure PageUp e aperte PageDown para descer.

## v8 — menu e modos
- Menu na tecla Insert: as setas escolhem e mudam as opções, e a escolha fica salva no .ini.
- Mod ativado/desativado pelo menu.
- Três modos: junto com a polícia (o original), substitui a polícia, ou o ciclo da v7.
- Trocar de modo ou desligar limpa tudo e devolve a polícia ao normal.
- Juggernaut humano com a armadura de Paleto Bay, copiada do jogo (segure PageDown e aperte End com o personagem de armadura). Sem a cópia, vem um mercenário com capacete e colete. O robô do Online saiu.

## v9 — chegada aos poucos
- Fila com espaçamento entre unidades (7–12 s no nível 1, 3,5–7 s no nível 8): fluxo contínuo em vez de bolo.
- Pressão crescente: o limite de inimigos parte de 4 e sobe até o do nível em cerca de 3 minutos.
- No máximo 3 + o nível de inimigos a menos de 60 m do jogador; o resto espera.
- 35% dos grupos a pé nascem mais longe.
- O tanque do Cargobob só atira depois de tocar o chão.
- Escala decrescente: a partir de 50 pontos, cada 5 pontos acima tiram 0,2 de cada ganho (mínimo de 0,2).
- Novas seções [Flow] e [Scaling] no .ini.
- Funciona longe da cidade: spawns mais longe, terreno irregular aceito no interior, grupos saindo da estrada mais próxima, mais helicópteros longe das estradas e barcos quando há água por perto.

## v9.2
- O mod vem desligado e o modo padrão é Junto com a polícia (aperte Insert e ative).
- Menu mais perto do topo, compacto, com letras maiores e título na fonte do GTA.
- Menu e avisos no idioma do jogo (português, espanhol ou inglês).

## v9.3
- Armas dos helicópteros funcionando: Annihilator, Valkyrie e helicópteros de ataque usam metralhadoras, canhões e mísseis; torretas tripuladas.
- Rapel: o helicóptero só joga as cordas parado e fica fixo até todos descerem.
- Juggernaut: armadura de Paleto guardada por protagonista, sorteada a cada Juggernaut, com aviso durante a missão.

## v10 — escala 500, combate mais próximo e eventos
- Escala de 0 a 500 pontos, níveis em 10, 80, 140, 200, 260, 320, 380 e 440. Ganhos: NPC +4, roubo +4, veículo +8, aeronave +12. Decaimento −15 a cada 45 s. Escala decrescente a partir de 250 (−0,25 a cada 25, mínimo 0,5). Timer de 5 min acima de 350.
- Facções: saíram FIB/IAA e os militares. Nível 6: segurança de Cayo Perico (cidade) e Caçadores (interior). Nível 7: mercenários russos do Bogdan. Nível 8: Merryweather.
- Veículos: saíram Menacer, Caracara, Halftrack, Barrage, Patrol Boat e Dinghy armado (o jogo expulsa o jogador da torreta) e os militares de Fort Zancudo. Entraram blindados (Insurgent, Kuruma blindado, Duke O'Death, Nightshark, Ardent, Dune FAV). Na torreta do Insurgent/Technical, ninguém tira o jogador e a tripulação inimiga desce.
- Sem armas Mk II. Acessórios aleatórios nas armas (lanterna, carregador, mira, empunhadura, supressor), de 10% no nível 1 a 80% no nível 8.
- Alcances de tiro: 35 m (curtas), 70 m (fuzis/metralhadoras), 110 m (RPG), 130 m (sniper), 100 m (veículos), 220 m (aeronaves). Fora do alcance, os inimigos a pé correm até o jogador.
- Inimigos sem visão do jogador (fora de cobertura) procuram um caminho até ele.
- Fora das estradas: IA agressiva e spawn nas estradas longe (até 650 m), vindo a pé.
- Perseguição na estrada (acima de 65 km/h): perseguidores atrás, na mesma mão; bloqueios mais frequentes e sempre na pista do jogador.
- Juggernaut: traje pesado do Online (camuflagens Noturno, Urbano, Aço, Tundra, Floresta), máscara de gás ou balística, capacete; 2.000 de vida; imune a corpo a corpo; caveira vermelha no mapa; minigun bem espalhada. Saiu a cópia da roupa de Paleto (tecla End).
- Blips por tipo (a pé, Juggernaut, carro, moto, barco, helicóptero, jato, tanque); raio de 180 m.
- Helicópteros armados só do nível 7 em diante, raros, só com o piloto atacando com as armas do helicóptero. Rapel mais comum e só com o jogador a pé.
- Tanques: um tiro a cada 10 s, mira fraca, Rhino mais alto no Cargobob, solto se o Cargobob perder o motor (PEM) ou baixar demais, nunca dentro do chão; tanque roubado atira. Novo tanque especial Khanjali (3%, por terra). Espera de 1 min entre tanques e entre caças. Caça mais raro.
- Modo avião: pilotando qualquer avião, só aviões armados perseguidores conforme o nível.
- Evento raro do Titan com paraquedistas (nível 7+, poucos inimigos vivos, jogador a pé, 10%).
- Limite de 30 inimigos ativos somando tudo.
- Menu: Trava de nível, Nível (até Máximo), Ciclo com nome curto, título centrado, suporte a controle (RB + ←).
- Ciclo: 5 estrelas por 5 min, a polícia sai e só volta quando a retaliação acaba.
- Modo Diretor: o modo do menu manda na polícia.
- Mira "stormtrooper" em tudo que mata rápido. Inimigos comuns também arrastam feridos.
- Novas seções/chaves no .ini: [Vehicles], [WeaponMods], [Paradrop], [Range] (Sniper, Rpg, Vehicle, Air, DragChance), [Tank] (ShotInterval, HangZ, KhanjaliChance, Cooldown), [Jet] Cooldown, [Heli] (AttackMinLevel, Accuracy), [Juggernaut] (Model, Blip, MinigunAccuracy), [General] LevelLock.

## v10.1 — armas de veículo travadas, Heavy e mar aberto
- Trava real das armas de veículo pela munição restrita (SET_VEHICLE_WEAPON_RESTRICTED_AMMO) e atributos de combate: Rhino 1 tiro a cada 15 s, Khanjali só na janela, helicópteros armados/aviões/caça/torretas em rajadas de 3 s com 8 s de pausa. Liberado ao soltar o veículo ou o jogador assumir.
- Juggernaut: ragdoll liga com menos de 350 de vida e na morte (o corpo não trava mais). A partir do nível 6, chance 18 (teto 40), caveira maior (escala 1,4).
- Heavy (Juggernaut leve), níveis 3–6: chefe da facção com 1000 de vida, colete 100, colete/máscara/chapéu/óculos sorteados, armas de cadência alta da facção com carregador estendido, num carro da facção. No nível 6 sorteia Heavy ou Juggernaut.
- Menos veículos pesados: helicóptero de ataque 3 (teto 6), Rhino 3 (teto 6), Khanjali 2, caça 2 (teto 5). Savage, Hunter e Akula mais raros que o Buzzard.
- Mar aberto (sem terra a 110 m): só helicóptero de ataque (nível 3+, sem rapel) e barcos inimigos.
- Civis e trânsito normais com estrelas ou retaliação; com a polícia junto, retaliadores em 75% do limite.
- Novas seções no .ini: [Heavy], [VehicleGuns] (FireSeconds, RestSeconds), [Population] (KeepCivilians, CiviliansPct, TrafficPct, WithPolicePct).

## v11 — modo Desordem
- Novo modo 4, Desordem (menu e Mode=4 no .ini). Começa no nível 3 com 3 estrelas, ou no nível escolhido no menu (nível N = N estrelas, máximo 5); nada baixa no primeiro minuto.
- Polícia e retaliadores se enfrentam. A polícia ignora o jogador (SET_POLICE_IGNORE_PLAYER, centro do procurado nele) e os retaliadores ficam neutros com ele, lutando contra os inimigos em volta (TASK_COMBAT_HATED_TARGETS_AROUND_PED). Quem chega a menos de 12 m ataca o jogador. Ferir um policial/viatura põe a polícia contra ele por 60 s; ferir um retaliador/veículo dele, os retaliadores.
- Qualquer morte (+2) ou veículo destruído (+4) a até 200 m do jogador sobe a pressão, de quem for. Estrelas acompanham o nível.
- Retaliadores odeiam os civis (e vice-versa). 15% dos civis recebem pistola, Micro SMG, Mini SMG ou Machine Pistol.
- Motoristas dos retaliadores dirigem até a área do jogador e a tropa desce a 60 m.
- Sem tanques (Rhino, Khanjali), caça e aviões perseguidores. Helicópteros com mísseis ou canhão explosivo: só o Buzzard (Valkyrie, Savage, Hunter e Akula trocados).
- Morrer ou ser preso recomeça a Desordem no nível inicial depois de 10 s.
- Nova seção [Disorder] no .ini: StartLevel, HoldSeconds, NearDist, AggroSeconds, DeathRadius, DeathPoints, VehiclePoints, ArmedCiviliansPct, RestartOnDeath, RestartDelay.

## v11.1 — alvos na Desordem e civis armados
- Desordem: ponto de encontro no jogador (centro do procurado nele). Cada retaliador ataca o policial ou civil armado mais perto (até 300 m); cada policial ataca o retaliador ou civil armado mais perto. Troca de alvo a cada 2,5 s se o alvo morrer ou houver outro mais perto. Sem alvo, o retaliador vem até ~30 m do jogador e luta com quem estiver em volta.
- Civis armados num grupo próprio (RET_ARMED_CIV): odeiam e são odiados pela polícia e pelos retaliadores, neutros com o jogador, sem fuga (flee 0) e sempre lutando (atributos 46 e 5).
- Modos 1, 2 e 3: polícia e retaliadores do mesmo lado (relação 1), todos contra o jogador. Só na Desordem eles se enfrentam.

## v11.2 — zona de combate na Desordem
- Polícia, retaliadores e civis armados só se enfrentam a até FightRadius (50 m) do jogador. Fora da zona, quem está a pé ignora o que vê (SET_BLOCKING_OF_NON_TEMPORARY_EVENTS) e anda até ~25 m do jogador; os alvos (policial, retaliador ou civil armado mais perto) só valem dentro da zona.
- Nova chave [Disorder] FightRadius=50.
