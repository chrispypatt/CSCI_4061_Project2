#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_TAB 100

extern int errno;

/*
 * Name:                uri_entered_cb
 * Input arguments:     'entry'-address bar where the url was entered
 *                      'data'-auxiliary data sent along with the event
 * Output arguments:    none
 * Function:            When the user hits the enter after entering the url
 *                      in the address bar, 'activate' event is generated
 *                      for the Widget Entry, for which 'uri_entered_cb'
 *                      callback is called. Controller-tab captures this event
 *                      and sends the browsing request to the ROUTER (parent)
 *                      process.
 */
void uri_entered_cb(GtkWidget* entry, gpointer data) {
	if(data == NULL) {
		return;
	}
	browser_window *b_window = (browser_window *)data;
	// This channel have pipes to communicate with ROUTER.
	comm_channel channel = b_window->channel;
	// Get the tab index where the URL is to be rendered
	int tab_index = query_tab_id_for_request(entry, data);
	if(tab_index <= 0) {
		fprintf(stderr, "Invalid tab index (%d).\n", tab_index);
		return;
	}
	// Get the URL.
	char * uri = get_entered_uri(entry);
	// Append your code here
	/*
	send NEW_URI_ENTERED request message to the ROUTER process
	message buff should contain three parts:
	1)type:NEW_URI_ENTERED
	2)uri: uri
	3)render_in_tab: tab_index
	*/
	child_req_to_parent *buff = (child_req_to_parent*) malloc(sizeof(child_req_to_parent));
	buff->type = 1;
	printf("%d uri_entered_cb \n", buff->type);
	strcpy(buff->req.uri_req.uri, uri);
	buff->req.uri_req.render_in_tab= tab_index;
	write(channel.child_to_parent_fd[1], buff, sizeof( child_req_to_parent));

}

/*
 * Name:                create_new_tab_cb
 * Input arguments:     'button' - whose click generated this callback
 *                      'data' - auxillary data passed along for handling
 *                      this event.
 * Output arguments:    none
 * Function:            This is the callback function for the 'create_new_tab'
 *                      event which is generated when the user clicks the '+'
 *                      button in the controller-tab. The controller-tab
 *                      redirects the request to the ROUTER (parent) process
 *                      which then creates a new child process for creating
 *                      and managing this new tab.
 */
void create_new_tab_cb(GtkButton *button, gpointer data)
{
	//printf("create new tab cb\n");
	if(data == NULL) {
		return;
	}
	browser_window *b_window = (browser_window *)data;
	// This channel have pipes to communicate with ROUTER.
	comm_channel channel = b_window->channel;
	// Append your code here
	//send CREATE_TAB type of message to ROUTER proecss
  	child_req_to_parent *buff = (child_req_to_parent*) malloc(sizeof(child_req_to_parent));
	buff->type = 0;

	write(channel.child_to_parent_fd[1], buff, sizeof(struct child_req_to_parent));
}

/*
 * Name:                url_rendering_process
 * Input arguments:     'tab_index': URL-RENDERING tab index
 *                      'channel': Includes pipes to communctaion with
 *                      Router process
 * Output arguments:    none
 * Function:            This function will make a URL-RENDRERING tab Note.
 *                      You need to use below functions to handle tab event.
 *                      1. process_all_gtk_events();
 *                      2. process_single_gtk_event();
*/
int url_rendering_process(int tab_index, comm_channel *channel) {

	browser_window * b_window = NULL;

	// Create url-rendering window
	create_browser(URL_RENDERING_TAB, tab_index, NULL, NULL, &b_window, channel);
	//child_req_to_parent req;
	fprintf(stderr,"tab %d\n", tab_index);
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	int flags;
	flags = fcntl(channel->parent_to_child_fd[0], F_GETFL);
	fcntl(channel->parent_to_child_fd[0], F_SETFL, flags | O_NONBLOCK);

	child_req_to_parent *buff= (child_req_to_parent*) malloc(sizeof(child_req_to_parent));

	//infinite loop to wait till the router writes the message for the rendering
	while (1) {
		/*
		when we read from the non-blocking pipe, there exists three situtations:
		1) nread = -1:
		error including EAGAIN, i.e., no data is immediately available for non-blocking reading
		2) nread = 0: end of file
		3) nread > 0: normal read into buff
		*/
		ssize_t nread= read(channel->parent_to_child_fd[0], buff, sizeof(child_req_to_parent));

		if (nread>0)
		{
			/*buff has two types
			- type = 1, we have to render the new url-rendering;
			- type = 2, kill the tab
			*/
			switch (buff->type){
	 			case 1: printf("NEW_URI_ENTERED\n");
					render_web_page_in_tab(buff->req.uri_req.uri, b_window);
	 				break;
	 			case 2: printf("TAB_KILLED");
					process_all_gtk_events();
					printf(" - url_rendering process received TAB_KILLED message\n");
					exit(0);
				default:
					printf("%d, other messages read - bogus\n", buff->type );
			}
		}
//no data was received from the ROUTER process
		else
			process_single_gtk_event();
			usleep(1000);

	}

	return 0;
}
/*
 * Name:                controller_process
 * Input arguments:     'channel': Includes pipes to communctaion with
 *                          Router process
 * Output arguments:    none
 * Function:        Router process
 * Output arguments:    none
 * Function:            This function will make a CONTROLLER window and
 *                      be blocked until the program terminates.
 */
int controller_process(comm_channel *channel) {
	// Do not need to change code in this function
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	browser_window * b_window = NULL;
	// Create controler window
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	show_browser();
	return 0;
}

/*
 * Name:                rouchannel[i]->child_to_parent_fd[0],buff,sizeof(struct child_req_to_parent))!=-1){ter_process
 * Input arguments:     none
 * Output arguments:    none
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminate.
 */
int router_process() {
	//each channel in the channel consists of two pipes for bi-directional communication
	comm_channel *channel[MAX_TAB];
	// Append your code here
	pid_t tab_pid[MAX_TAB];
	int open_tabs = 0;
	int closed_tab;
	int flags;
	bool receive;
	child_req_to_parent *buff=(child_req_to_parent*) malloc(sizeof(child_req_to_parent));
	int i;
	for (i = 0; i<MAX_TAB;i++){
		channel[i]=NULL;
	}
	// Prepare communication pipes with the CONTROLLER process
	channel[0] =(comm_channel*) malloc(sizeof(comm_channel));
	pipe(channel[0]->parent_to_child_fd);
	pipe(channel[0]->child_to_parent_fd);

	// Fork the CONTROLLER process
	//   call controller_process() in the forked CONTROLLER process
	if ((tab_pid[0] = fork()) == 0){
		controller_process(channel[0]);
		exit(0);
	}
	//parent process serves as message receiver
	close(channel[0]->child_to_parent_fd[1]);
	close(channel[0]->parent_to_child_fd[0]);
		// Poll child processes' communication channels using non-blocking pipes.
		while(1){
			int i;
			//set boolean variable receive =false, and if none of channels receive massage, sleep
			receive= false;
			for (i=0;i<=open_tabs;i++){
				if (channel[i]!=NULL){
					//configure the pipe as the non-blocking pipe
					flags = fcntl(channel[i]->child_to_parent_fd[0], F_GETFL);
					fcntl(channel[i]->child_to_parent_fd[0], F_SETFL, flags | O_NONBLOCK);
					/*
					when we read from the non-blocking pipe, there exists three situtations:
					1)error including EAGAIN, i.e., no data is immediately available for non-blocking reading: nread = -1
					2)end of file: nread == 0
					3)normal read into buff: nread > 0
					*/
					ssize_t nread= read(channel[i]->child_to_parent_fd[0],buff,sizeof(struct child_req_to_parent));

					if (nread>0){
						receive= true;
						switch (buff->type){

							 /*
							 message type 0: CREATE_TAB
							 read from controller_process and
							 create a new url_rendering_process & tab & channel
							 */
							case 0:
								if (i==0){
									open_tabs++;
									channel[open_tabs] =(comm_channel*) malloc(sizeof(comm_channel));
									pipe(channel[open_tabs]->parent_to_child_fd);
									pipe(channel[open_tabs]->child_to_parent_fd);
									if ((tab_pid[open_tabs]=fork()) == 0){
										printf("CREATE_TAB, open_tabs %d\n", open_tabs);
										url_rendering_process(open_tabs,channel[open_tabs]);
										exit(0);
									}
									else {
										close(channel[open_tabs]->child_to_parent_fd[1]);
										close(channel[open_tabs]->parent_to_child_fd[0]);
										break;
									}
							}
								/*
								message type 1: NEW_URI_ENTERED
								read url from controller_process and write to url_rendering_process
								Send message to the URL-RENDERING process in which the new url is going to be rendered
								*/
							case 1:
								if (i==0){
									printf("NEW_URI_ENTERED\n");
									write(channel[buff->req.uri_req.render_in_tab]->parent_to_child_fd[1], buff, sizeof(child_req_to_parent) );
									break;
								}
							/*
							message type 2:TAB_KILLED:
							If the killed process is the CONTROLLER process, send messages to kill all URL-RENDERING processes
							If the killed process is a URL-RENDERING process, send message to the URL-RENDERING to kill it
							*/
							case 2:// printf("cleaning up\n");
							//Cleaning up by deleting the pipes
							closed_tab= buff->req.killed_req.tab_index;
							/*
							If the killed process is the CONTROLLER process,
							send messages to kill all URL-RENDERING processes.
							send TAB_KILLED to every alive url_rendering_process,
							then exit(0)
							*/
								if (closed_tab==0){
									/*
									once recieved TAB_KILLED message from CONTROLLER_TAB
									send TAB_KILLED to every alive url_rendering_process,
									close its pipe and remove it from ROUTER's pipe list.
									then exit(0)
									*/
									for (i=1;i<=open_tabs;i++){
										if (channel[i]!=NULL){
											if (write(channel[i]->parent_to_child_fd[1],buff,sizeof(struct child_req_to_parent))!=-1){
												if (waitpid(tab_pid[i], NULL, 0)>0){
													close(channel[i]->child_to_parent_fd[0]);
													close(channel[i]->parent_to_child_fd[1]);
													channel[i]=NULL;
												}
												else
													printf("error in waitpid() in terms of tab_index %d when closing CONTROLLER_TAB \n",i);
											}
										}
											else printf("\n Failed to send kill message to child process: Bad file descriptor or tab has previously been killed\n");

								}
								exit(0);
							}
							/*
							If the killed process is a URL-RENDERING process,
							send message to the URL-RENDERING to kill it.
							send TAB_KILLED to this url_rendering_process, if alive.
							close its pipe and remove it from ROUTER's pipe list.
							then break.
							*/
							else{
								if (channel[closed_tab]!=NULL){
									if (write(channel[closed_tab]->parent_to_child_fd[1],buff,sizeof(struct child_req_to_parent))!=-1){
										if (waitpid(tab_pid[closed_tab],NULL,0)>0){
										close(channel[closed_tab]->child_to_parent_fd[0]);
										close(channel[closed_tab]->parent_to_child_fd[1]);
										channel[closed_tab]=NULL;
										}
										else
											printf("error in waitpid() in terms of tab_index %d when closing URL_RENDERING_TAB \n",closed_tab);
									}
							}
								break;
						}
					}
				}
			}
	 }
	 //sleep some time if no message received
	 if (receive== false)
	 	usleep(1000);
 }
 return 0;
}

int main() {
	return router_process();
}
