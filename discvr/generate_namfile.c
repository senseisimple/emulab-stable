#include "discvr.h"
#include "packet.h"
#include "util.h"
#include "generate_namfile.h"

u_char *mac_list[MAX_NODES];
int num_nodes=0;
db_type mac_db[40];

link_type *links_list; 

void
read_mac_db()
{
	FILE *dbfptr=NULL;
	char hostname[HOSTNAME_LEN];
	char if_addr[20];
	int if_num=0,i=0;
	if((dbfptr = fopen("MAC_DB","r")) == NULL)
	{
        perror("MAC_DB file not found\n");
		exit(1);
	}
	i=0;
    while(i<40)
	{
		if(fscanf(dbfptr,"%s\t%d\t%s\n",hostname,&if_num,if_addr)==0)
			break;
		//printf("values read: \"%s\" \"%d\" \"%s\"\n",hostname,if_num,if_addr);
		mac_db[i].hostname = (char*)malloc(sizeof(char)*strlen(hostname));
		strcpy(mac_db[i].hostname,hostname);
		strcpy(mac_db[i].addrs[if_num],if_addr);
		fscanf(dbfptr,"%s\t%d\t%s\n",hostname,&if_num,if_addr);
		strcpy(mac_db[i].addrs[if_num],if_addr);
		fscanf(dbfptr,"%s\t%d\t%s\n",hostname,&if_num,if_addr);
		strcpy(mac_db[i].addrs[if_num],if_addr);
		fscanf(dbfptr,"%s\t%d\t%s\n",hostname,&if_num,if_addr);
		strcpy(mac_db[i].addrs[if_num],if_addr);
		fscanf(dbfptr,"%s\t%d\t%s\n",hostname,&if_num,if_addr);
		strcpy(mac_db[i].addrs[if_num],if_addr);
		i++;
	}
	fclose(dbfptr);
}

void
get_hostname(char *addr,char *hostname)
{
	int i=0,j=0;

	for(i=0;i<40;i++)
	{
		for(j=0;j<5;j++)
		{
			if(strcmp(addr,mac_db[i].addrs[j])==0)
			{
				strcpy(hostname,mac_db[i].hostname);
				return;
			}
		}
	}
	printf("Error:Couldnt find the mac_address in the db\n");
	strcpy(hostname,"dontknow");
}


/* Will need to change this function. Change the data structure to have the link
 * as a hash key. Two nodes may have multiple links between them.
 */
void
gen_nam_file(const char *mesg, size_t nbytes,char *file_name)
{
	struct topd_nbor *p;
	FILE *fptr=NULL;
	char addr[MAC_ADDR_STR_LEN];
	char hostname[HOSTNAME_LEN];
	int lan_count=MAX_NODES;
	int node_count=0;
	link_type * temp_ptr;
	lan_node_type *temp_ptr2;
	/* Read the MAC address and Hostname mapping from the "MAC_DB" file into 
	 * mac_db array.
	 */
	read_mac_db();

	/* Open the output nam file for writing the topology informantion
	 */
	if((fptr = fopen(file_name,"w")) == NULL)
		perror("Error:fopen");
	fprintf(fptr,"V -t * -v 1.0a5 -a 0\n");

	p = (struct topd_nbor *) (mesg + sizeof(topd_inqid_t));
	num_nodes=0;
	/* Go through the list of links received by the client as reply to its query
	 * one by one.
	 */
	while((char *)p < mesg + nbytes ) 
	{
		add_link(p);
		print_links_list();
		/* Check that this node's node_id is not there in the
		 * mac_list already. If its already there in the list then don't add to the 
		 * list again. 
		 */
		if(not_in_node_list(p->tdnbor_pnode))
		{
			mac_list[num_nodes]=(u_char*)malloc(sizeof(u_char)*ETHADDRSIZ);
			memcpy(mac_list[num_nodes],p->tdnbor_pnode, ETHADDRSIZ);
			/* Get the MAC address as a string. Convert the MAC address in the
			 * hexadecimal format to string format
			 */
			get_mac_addr(p->tdnbor_pnode,ETHADDRSIZ, addr);
			/* Use the mac_db to convert the mac address to host name
			 */
			get_hostname(addr,hostname);
			fprintf(fptr, "n -t * -s %d -v circle -c black -e red -l %s\n",num_nodes, hostname);
			num_nodes++;
		}
		//ptr_link_struct = 
		p++;
	}
	
	/* Set the version number of nam file
	 */
	

	for(temp_ptr = links_list; temp_ptr!=NULL;temp_ptr = temp_ptr->next)
	{
		if(temp_ptr->lan_if_list==NULL)
			fprintf(fptr,"l -t * -s %d -d %d -c blue\n",
					get_id(temp_ptr->pointA),get_id(temp_ptr->pointB));
	}
	lan_count = num_nodes;
	for(temp_ptr = links_list; temp_ptr!=NULL;temp_ptr = temp_ptr->next)
	{
		if(temp_ptr->lan_if_list!=NULL)
		{
			fprintf(fptr,"X -t * -n %d -o left\n",lan_count);
			fprintf(fptr,"L -t * -s %d -d %d -o up\n",
					lan_count,get_id(temp_ptr->pointA));
			for(node_count=0,temp_ptr2=temp_ptr->lan_if_list;temp_ptr2!=NULL;
				temp_ptr2=temp_ptr2->next,node_count++)
			{
				fprintf(fptr,"L -t * -s %d -d %d ",
						lan_count,get_id(temp_ptr2->node));
				if(node_count%2==0)
					fprintf(fptr,"-o down\n");
				else
					fprintf(fptr,"-o up\n");
			}
			lan_count++;
		}
	}
	fprintf(fptr,"\n");
	fclose(fptr);
}

int
get_id(u_char *mac_addr)
{
	int i=0;
	char str_mac_addr[MAC_ADDR_STR_LEN],str_mac_addr2[MAC_ADDR_STR_LEN];
	char hostname[HOSTNAME_LEN], hostname2[HOSTNAME_LEN];

	get_mac_addr(mac_addr,ETHADDRSIZ, str_mac_addr);
	get_hostname(str_mac_addr,hostname);
	for(i=0;i<num_nodes;i++)
	{
		get_mac_addr(mac_list[i],ETHADDRSIZ, str_mac_addr2);
		get_hostname(str_mac_addr2,hostname2);

		if(strcmp(hostname,hostname2)==0)
			return i;
	}
	printf("Error: Could not find \"%s\" mac address in the list\n", mac_addr);
	return MAX_NODES;
}

int 
not_in_node_list(u_char *mac_addr)
{
	int i=0;
	for(i=0;i<num_nodes;i++)
	{
		if(memcmp(mac_addr,mac_list[i],ETHADDRSIZ)==0)
            return 0;
	}
	return 1;
}

/*  Add link function adds a link to the links_list data structure which is the
 *  list of all the links in the topology. 
 *   - It ignores the link if its a duplicate to any of the existing link
 *   - If one of the interface is common to two links then the links are part
 *     of a LAN. Since the query is forwarded on the LAN from a single interface
 *     of a node hence for a LAN there is just one common interface. This
 *     interface is seperately (pointA of link structure) for making the
 *     comparisions simple.
 *     The rest of the interfaces on the LAN is stored as a link list of nodes and
 *    pointed by the "lan_if_list" in the link structure.
 */


void 
add_link(struct topd_nbor *p)
{
	link_type * temp_ptr;
	lan_node_type *temp_ptr_lan;
	int w,x,y,z;

	u_char zero_mac[ETHADDRSIZ];
	bzero(zero_mac,ETHADDRSIZ);

	if(memcmp(zero_mac,p->tdnbor_dnode,ETHADDRSIZ)==0)
		return;
	printf("adding...\n");
	for(temp_ptr = links_list;temp_ptr!=NULL;temp_ptr = temp_ptr->next)
	{
		w = memcmp(temp_ptr->pointA,p->tdnbor_dnode,ETHADDRSIZ);
		x = memcmp(temp_ptr->pointA,p->tdnbor_dif,ETHADDRSIZ);

		if(temp_ptr->pointB != NULL)
		{
			y = memcmp(temp_ptr->pointB,p->tdnbor_dnode,ETHADDRSIZ);
			z = memcmp(temp_ptr->pointB,p->tdnbor_dif,ETHADDRSIZ);
		}
		else
		{
			y=1;z=1;
		}
		
		if((w==0 && z==0) || (x==0 && y==0))
		{
			printf("Link already there in the list\n");
			return;
		}
		else if((w==0 && z!=0) || (w!=0 && z==0) || (x==0 && y!=0) || (x!=0 && y==0))
		{
			printf("lan link found\n");
			if(temp_ptr->lan_if_list==NULL)
			{
				/* Store the common interface in pointA, copy interface in pointB
				 * to a lan_node and set pointB to NULL
				 */
				if(y==0 || z==0)
				{
					temp_ptr->lan_if_list =
						(lan_node_type*)malloc(sizeof(lan_node_type));
					memcpy(temp_ptr->lan_if_list->node,temp_ptr->pointA,ETHADDRSIZ);
					memcpy(temp_ptr->pointA,temp_ptr->pointB,ETHADDRSIZ);
					free(temp_ptr->pointB);
					temp_ptr->pointB=NULL;
				}
				else
				{
					temp_ptr->lan_if_list =
                        (lan_node_type*)malloc(sizeof(lan_node_type));
                    memcpy(temp_ptr->lan_if_list->node,temp_ptr->pointB,ETHADDRSIZ);
					free(temp_ptr->pointB);
					temp_ptr->pointB=NULL;
				}
				
			}
			temp_ptr_lan = temp_ptr->lan_if_list;
			temp_ptr->lan_if_list = (lan_node_type*)malloc(sizeof(lan_node_type));
			if(x==0 || z==0)
				memcpy(temp_ptr->lan_if_list->node,p->tdnbor_dnode,ETHADDRSIZ);
			else if(w==0 || y==0)
				memcpy(temp_ptr->lan_if_list->node,p->tdnbor_dif,ETHADDRSIZ);
			temp_ptr->lan_if_list->next = temp_ptr_lan;
			/*
			for(temp_ptr2=temp_ptr->lan_if_list;temp_ptr2!=NULL;
				temp_ptr2 = temp_ptr2->next);
			temp_ptr2=(lan_node_type*)malloc(sizeof(lan_node_type));
			memcpy(temp_ptr2->pointA,p->tdnbor_dnode,ETHADDRSIZ);
			memcpy(temp_ptr2->pointB,p->tdnbor_dif,ETHADDRSIZ);
			temp_ptr2->lan_if_list = NULL;
			temp_ptr2->next = NULL;
			*/
		return;
		}
	}
	temp_ptr = links_list;
	links_list=(link_type*)malloc(sizeof(link_type));
	links_list->pointA = (u_char*)malloc(ETHADDRSIZ*sizeof(u_char));
	links_list->pointB = (u_char*)malloc(ETHADDRSIZ*sizeof(u_char));
	printf("copying:A->");
	print_haddr(p->tdnbor_dnode,ETHADDRSIZ);
	printf(" # B->");
	print_haddr(p->tdnbor_dif,ETHADDRSIZ);
	printf("\n");
	memcpy(links_list->pointA,p->tdnbor_dnode,ETHADDRSIZ);
	memcpy(links_list->pointB,p->tdnbor_dif,ETHADDRSIZ);
	printf("copied:A->");
	print_haddr(links_list->pointA,ETHADDRSIZ);
	printf(" # B->");
	print_haddr(links_list->pointB,ETHADDRSIZ);
	printf("\n");
	links_list->lan_if_list = NULL;
	links_list->next = temp_ptr;
}

void
print_links_list()
{
	link_type *temp_ptr;
	lan_node_type *temp_ptr2;
	printf("Links collected:\n");
	printf("---------------------------------------------------\n");

	for(temp_ptr = links_list; temp_ptr!=NULL;temp_ptr = temp_ptr->next)
	{
		if(temp_ptr->lan_if_list==NULL)
		{
			printf("[");
			print_haddr(temp_ptr->pointA,ETHADDRSIZ);
			printf("<->");
			print_haddr(temp_ptr->pointB,ETHADDRSIZ);
			printf("]");
			printf("\n");
		}
		else
		{
			printf("[");
			print_haddr(temp_ptr->pointA,ETHADDRSIZ);
			printf(" # ");
			for(temp_ptr2=temp_ptr->lan_if_list;temp_ptr2->next!=NULL;
				temp_ptr2=temp_ptr2->next)
			{
				print_haddr(temp_ptr2->node,ETHADDRSIZ);
				printf(" # ");
			}
			print_haddr(temp_ptr2->node,ETHADDRSIZ);
			printf("]");
			printf("\n");
		}
	}
	printf("---------------------------------------------------\n");
}

