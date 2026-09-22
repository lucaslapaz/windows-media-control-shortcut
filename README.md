# Media Control

Teclado sem teclas de mídia? O Media Control resolve isso.

Ele fica em segundo plano e transforma o **Ctrl direito** combinado com as teclas
de função nos comandos de mídia que faltam no seu teclado: pausar, pular faixa,
mudar o volume e mutar. Os atalhos funcionam em qualquer aplicativo, sem console
aberto e sem janela atrapalhando.

## Atalhos

| Atalho                | O que faz                   |
| --------------------- | --------------------------- |
| `Ctrl direito` + `F5` | Reproduzir ou pausar        |
| `Ctrl direito` + `F6` | Faixa anterior              |
| `Ctrl direito` + `F7` | Próxima faixa               |
| `Ctrl direito` + `F8` | Diminuir o volume           |
| `Ctrl direito` + `F9` | Aumentar o volume           |
| `Ctrl direito` + `F10`| Ativar ou desativar o mudo  |

Só o **Ctrl direito** dispara os atalhos. O Ctrl esquerdo continua livre, então
`Ctrl+F5` no navegador, `Ctrl+F6` no editor e companhia seguem funcionando
normalmente.

## Como usar

1. Abra o `MediaControl.exe`. Nada aparece na tela: o programa vai direto para a
   bandeja do sistema, ao lado do relógio.
2. Use os atalhos em qualquer aplicativo.
3. Clique no ícone da bandeja para abrir o menu:
   - **Atalhos e ajuda** — abre uma janela com a lista de atalhos e uma
     explicação rápida;
   - **Sair** — encerra o programa.

Abrir o programa uma segunda vez não cria outra cópia: a que já está rodando
apenas mostra a janela de ajuda.

### Iniciar junto com o Windows

Pressione `Win+R`, digite `shell:startup` e coloque um atalho para o
`MediaControl.exe` na pasta que abrir.

## Compilando

Requisitos:

- **CMake** 3.21 ou mais novo
- **Ninja**
- Um compilador C++20 para Windows — MinGW-w64 (GCC 13+) ou MSVC (Visual Studio
  2019 ou mais novo)

O projeto compila sem avisos nos dois compiladores, com `/W4` no MSVC e
`-Wall -Wextra -Wpedantic -Wshadow` no GCC.

### MinGW-w64

```sh
cmake --preset mingw
cmake --build --preset mingw
```

O executável sai em `build/mingw/MediaControl.exe`.

### MSVC

Rode os comandos a partir de um **Developer Command Prompt** (ou depois de
executar o `vcvars64.bat`), para que o `cl.exe` esteja no `PATH`:

```sh
cmake --preset msvc
cmake --build --preset msvc
```

O executável sai em `build/msvc/Release/MediaControl.exe`.

### Sobre o executável

O runtime é ligado estaticamente, então o `.exe` roda sozinho, sem DLLs ao lado.
Se preferir o runtime dinâmico, configure com
`-DMEDIACONTROL_STATIC_RUNTIME=OFF`.

## Como funciona

**Por que um hook de teclado e não `RegisterHotKey`?**
Porque `RegisterHotKey` não distingue o Ctrl esquerdo do direito. Registrar
`Ctrl+F5` tomaria o atalho de todos os aplicativos do sistema — exatamente o que
este programa evita. Um hook de baixo nível (`WH_KEYBOARD_LL`) enxerga qual das
duas teclas foi pressionada.

**O que acontece quando o atalho é reconhecido?** O programa injeta a tecla de
mídia correspondente (`VK_MEDIA_PLAY_PAUSE`, `VK_VOLUME_UP` e companhia) com
`SendInput`, como faria um teclado que tem essas teclas de verdade. Daí em
diante quem decide é o Windows: ele entrega o comando ao aplicativo que está com
a sessão de mídia e mostra o próprio balão de volume.

**O atalho não vaza para o aplicativo em foco.** Quando o chord é reconhecido, a
tecla é consumida pelo hook — tanto o pressionar quanto o soltar, para que
nenhuma janela receba um evento pela metade.

**O hook não se alimenta.** Cada tecla injetada pelo programa leva uma marca em
`dwExtraInfo`, e o hook ignora o que carrega essa marca. Isso é mais preciso do
que descartar tudo que for sintético: teclas vindas de área de trabalho remota,
teclado virtual ou macros continuam acionando os atalhos.

**O hook não trava o teclado.** O callback do hook roda no caminho de todas as
teclas do sistema, então ele só publica uma mensagem para a janela do programa;
o trabalho de verdade acontece no laço de mensagens.

## Estrutura do projeto

```
assets/
  app.ico              ícone multirresolução (16 a 256 px)
src/
  shortcuts.hpp        a tabela de atalhos — fonte única da verdade
  hotkey_listener.*    o hook de teclado que reconhece os atalhos
  media_command.*      envio das teclas de mídia via SendInput
  tray_icon.*          o ícone na bandeja, com ciclo de vida RAII
  help_window.*        a janela de atalhos e ajuda, desenhada à mão
  application.*        janela oculta, menu da bandeja e laço de mensagens
  main.cpp             ponto de entrada e instância única
  app.rc               ícone, manifesto e informações de versão
  app.manifest         DPI por monitor, common controls v6, asInvoker
```

A janela de ajuda lê a mesma tabela `kShortcuts` que o hook usa, então o que o
programa faz e o que ele documenta não têm como divergir. Ela também acompanha o
tema claro/escuro do Windows e se redesenha ao mudar de monitor com DPI
diferente.

## Requisitos para rodar

Windows 10 versão 1607 ou mais novo, 64 bits.

## Créditos

O ícone vem do tema **Nuvola**, de David Vignoni, distribuído sob LGPL. Os
detalhes estão em [`assets/ICON-LICENSE.md`](assets/ICON-LICENSE.md).
