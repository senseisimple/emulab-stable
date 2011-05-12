system("../../../pubsub/pubsubd -v -d -p 4001");
system("./simple-agent -E delay-agent/single-node -s localhost -k /var/emulab/boot/eventkey -u foobar -p 4001");
