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
