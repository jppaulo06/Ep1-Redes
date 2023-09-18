if [[ $# -ne 1 ]]; then
  printf "ERROR!\nUsage: ./test.sh <queu-name>\n"
  exit 1
fi
amqp-declare-queue -q "$@"
