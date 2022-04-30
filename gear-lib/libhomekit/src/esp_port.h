#define esp_random()        random()
#define esp_restart()               printf("esp_restart\n")
#define mdns_init()                 printf("mdns_init\n")
#define mdns_hostname_set(name)      printf("mdns_hostname_set %s\n", name)
#define mdns_instance_name_set(name) printf("mdns_instance_name_set %s\n", name)
#define mdns_service_add(inst, svc, proto, port, txt, num)       \
                printf("mdns_service_add %s %s %s %d %p %d\n", \
                        inst, svc, proto, port, txt, num)
#define mdns_service_txt_item_set(svc, proto, key, val) printf("mdns_service_txt_item_set %s %s %s %s \n", svc, proto, key, val)
