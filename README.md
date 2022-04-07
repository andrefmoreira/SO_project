# SO_project



MOBILE NODES:
- Criação do mobile node
- Leitura correta dos parâmetros da linha de comando
- Geração e escrita das tarefas no named pipe



SYSTEM MANAGER:
- Arranque do simulador de offloading, leitura do ficheiro de configurações,
validação dos dados do ficheiro e aplicação das configurações lidas
- Criação da memória partilhada
- Criação do named pipe
- Criação dos processos Task Manager, Monitor e Maintenance Manager
- Criação da fila de mensagens
- Escrever a informação estatística no ecrã como resposta ao sinal SIGTSTP
- Capturar o sinal SIGINT, terminar a corrida e liberta os recursos



TASK MANAGER:
- Criar os processos Edge Server de acordo com as configurações
- Ler e validar comandos lidos do named pipe
- Criação da thread scheduler e gestão do escalonamento das tarefas
- Criação da thread dispatcher para distribuição das tarefas



EDGE SERVER:
- Criação das threads que simulam os vCPUs
- Executar as tarefas



MONITOR:
- Controla o nível de performance dos Edge Server de acordo com as regras
estabelecidas



MAINTENANCE MANAGER:
- Gerar mensagens de manutenção, receber resposta e gerir a manutenção



FICHEIRO LOG:
- Envio sincronizado do output para ficheiro de log e ecrã



GERAL:
- Criar um makefile
- Diagrama com a arquitetura e mecanismos de sincronização
- Suporte de concorrência no tratamento de pedidos
- Deteção e tratamento de erros
- Atualização da shm por todos os processos e threads que necessitem
- Sincronização com mecanismos adequados (semáforos, mutexes ou variáveis de
condição)
- Prevenção de interrupções indesejadas por sinais não especificados no enunciado;
fornecer a resposta adequada aos vários sinais especificados no enunciado
- Após receção de SIGINT, terminação controlada de todos os processos e threads, e
libertação de todos os recursos
