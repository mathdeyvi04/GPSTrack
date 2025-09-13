# Sumário

- [Objetivo](#objetivo)
- [Caso Deseje **Contribuir**](#caso-deseje-contribuir)
- [Caso Deseje **Utilizar**](#caso-deseje-utilizar)
- [Descrições Sucintas de Código](#descrições-sucintas-de-código)
- [Confirmação de Leitura de Dados](#confirmação-de-leitura-de-dados)

# Objetivo

Desenvolver uma solução embarcada usando o kit de desenvolvimento **STM32MP1 DK1** e
o sensor **GY-GPS6MV2**, a fim de monitorar de forma remota e contínua qualquer tentativa de violação e/ou comprometimento de uma carga.

# Caso Deseje Contribuir

Supondo que você deseje ser um contribuinte, atente-se às necessidades da aplicação:

### Conhecimentos a cerca do Sensor GPS **GY-GPS6MV2**:

Sugiro a observação do vídeo [linkado](https://youtu.be/lZumBl7zhoM) e busca inteligente no chatgpt.

### Kit de Desenvolvimento STM32

O sensor deve ser conectado à placa da seguinte forma:

![](https://github.com/user-attachments/assets/ea1d0935-fcd3-4160-b46f-a917f578f33d)

Além do compilador padrão para verificações superficiais, será necessário um compilador específico para o microcontrolador que estamos focando. 

- Download do Compilador Cross-Plataform

Verifique o [link](https://drive.google.com/file/d/1qpq3QeK5f7T061LFA0JlJz2fgMQDvyMn/view?usp=drivesdk) dado pelo professor e realize:

```
tar -xvf arm-buildroot-linux-gnueabihf_sdk-buildroot.tar.gz
```

Isso descompactará o kit de desenvolvimento da placa, permitindo diversas funcionalidades. 

> [!WARNING]
> Não se preocupe com o tempo que demorará.

Ao realizar a descompactação, você terá:

```
arm-buildroot-linux-gnueabihf_sdk-buildroot/
    |-- bin/
    │   |-- arm-buildroot-linux-gnueabihf-gcc
    │   |-- arm-buildroot-linux-gnueabihf-g++
    |   ...
    |-- arm-buildroot-linux-gnueabihf/
    |   |-- sysroot
    |   ...
    |
    ...
```

Observe na pasta `bin` a presente dos compiladores `gcc` e `g++`.

### Makefile

Caso fôssemos utilizar apenas o compilador, precisaríamos escrever comandos grandes e fáceis de 
errar. Sendo assim, faz-se necessário um intermediário:

```
sudo apt install make
```

### Doxygen

Utilizaremos esse software para criar documentações de forma automática. 
**Não é necessário que todos os contribuintes possuam essa ferramenta**,
entretanto, é estritamente obrigatório a utilização do mesmo padrão de documentação.

```
sudo apt install doxygen graphviz
sudo apt install texlive-latex-extra texlive-fonts-recommended texlive-fonts-extra texlive-latex-recommended
```

O segundo download faz referência ao construção do arquivo `.pdf`.

### Acesso à Placa

Utilizaremos o protocolo SSH para acessá-la remotamente. Naturalmente,
será necessário um cabo Ethernet e que os IP's estejam na mesma faixa.
É interessante que você utilize a função _ping_ para verificar se estão conectadas corretamente.

Assim que a conexão for confirmada, dentro do PowerShell, execute:

```
ssh root@192.168.42.2
```

Será solicitado a senha e, posteriormente, o acesso será permitido.

Para enviar arquivos para a placa, volte novamente ao PowerShell e:

```
scp -O GPSTrack root@192.168.42.2:/
```

Assim, o arquivo estará no mesmo local que as pastas root do sistema linux. Além disso, pode ser necessário dar permissão de execução ao binário, para tanto:

```
chmod +x GPSTrack
```

# Caso Deseje Utilizar

A seguir, regras de compilação e execução.

### `GPSTrack`

O arquivo binário `GPSTrack` presente neste diretório já representa o executável a ser inserido na placa para completa operacionalidade.

Com a adição da feature de envio via rede, torna-se necessário a entrada de novos argumentos, o _ip e a porta de destino_.

Sendo assim, a execução fica, por exemplo: `./GPSTrack 127.0.0.1 1234`.

### `make`

Compilará a aplicação utilizando as flags necessárias e o compilador específico, gerando 
um executável de pronto uso no módulo.

### `make debug` 

Compilará a aplicação para o Linux, executará e apagará o executável gerado.
Neste caso, há um simulador para o módulo GPS que estaremos usando a fim de que 
possamos realmente realizar testes.

Esse modo também é interessante para aqueles que não possuem o sensor, nem a placa. Neste caso, 
a aplicação via as informações para o localhost e para a porta 9000.

### `make docs`

Para contribuintes, gerará um PDF contendo a documentação da aplicação geral.

# Descrições Sucintas de Código

### GPSTrack

Classe responsável por:

- Ler dados do sensor GPS6MV2
- Interpretar os dados lidos
- Enviar as informações 

Cada uma dessas fases ocorrem em uma thread dedicada controlada por métodos `init` e `stop`. Utilizamos a thread para conseguir controle absoluto sobre tempo de execução.

Fluxo de Funcionamento:

- Inicialização

A classe configura um socket UDP para a transmissão dos dados processados e uma porta serial, na qual o sensor GPS 6MV2 enviará os dados.

- Leitura dos Dados:

O método `read_serial` coleta os dados diretamente da porta serial.

- Interpretação dos Dados:

A classe `GPSData` é completamente responsável pelo parsing dos dados, conseguindo, por enquanto, traduzir apenas as mensagens no estilo _GPGGA_, a partir da qual podemos obter com
segurança informações de horário em UTC, latitude, longitude e altitude.

- Envio de informações:

A função `send` envia as informações via socket UDP para uma determinada máquina e porta.

Para informações mais precisas e profundas, sugiro verificar o arquivo 
[index.html](docs/html/index.html) ou [Documentation.pdf](Documentation.pdf), sendo este último gerado pelo comando `make docs`.

# Confirmação de Leitura de Dados

Como nem todas as placas são iguais, não como definir com propriedade o procedimento para visualização dos dados. 

Em nosso caso, o sensor conectou-se pelo terminal serial `/dev/ttySTM2`.

### Caso deseje ver a comunicação entre o sensor e a placa em tempo real

```
cat /dev/ttySTM2
```

Dependendo de quando o comando foi executado, diferentes formas de texto surgirão.

- Caso tenha sido imediatamente após ligá-lo:

![](https://github.com/user-attachments/assets/147db746-8515-40b2-8a8a-10a0ac17ed0e)

- Caso tenha sido após tempo _suficientemente_ longo e em local aberto:

![](https://github.com/user-attachments/assets/c890a3d0-9aae-40e2-99f5-d13a59aba50e)

### Para executar a aplicação

Considerando que um determinado tempo foi esperado, execute:

```
./GPSTrack
```

Então as seguintes mensagens devem surgir à tela:

![](https://github.com/user-attachments/assets/2acdb632-2ac4-4a12-98da-76a99bce8713)

Observe como nossa aplicação apenas interpreta o padrão _GGA_ lançado pelo sensor, retornando
as informações de _Hora_em_UTC_, _Latitude_, _Longitude_, _Altitude_.

### Visualização na Máquina Conectada

Utilizando NetCat para filtrar as entradas em uma porta específica da máquina, é possível ver o fluxo de dados entrando no servidor.

![](https://github.com/user-attachments/assets/ae476e05-20e1-4470-b4e1-daf2db0c7e3e)

