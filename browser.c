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
	comm_channel channel = ((browser_window*)data)->channel;
	// Get the tab index where the URL is to be rendered
	int tab_index = query_tab_id_for_request(entry, data);
	if(tab_index <= 0) {
		fprintf(stderr, "Invalid tab index (%d).", tab_index);
		return;
	}
	// Get the URL.
	char * uri = get_entered_uri(entry);
	// Append your codemm_channel *channel[MAX_TAB] here
	// Send 'which' message to 'which' process?
	//
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
	if(data == NULL) {
		return;
	}
	browser_window *b_window = (browser_window *)data;
	// This channel have pipes to communicate with ROUTER.
	comm_channel channel = b_window->channel;
	// Append your code here
	// Send 'which' message to 'which' process?
	//
  child_req_to_parent *buff=(child_req_to_parent*) malloc(sizeof(child_req_to_parent));
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
	child_req_to_parent req;
	fprintf(stderr,"tab %d\n", tab_index);
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	while (1) {
		usleep(1000);
		// Append your code here
		// Handle 'which' messages?
		//
		process_single_gtk_event();

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
	comm_channel *channel[MAX_TAB];
	// Append your code here
	pid_t tab_pid[MAX_TAB];
	int open_tabs = 0;
	int flags;
	child_req_to_parent *buff=(child_req_to_parent*) malloc(sizeof(child_req_to_parent));
	for (int i = 0; i<MAX_TAB;i++){
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


	// Poll child processes' communication channels using non-blocking pipes.
while(1){
	for (int i=0;i<=open_tabs;i++){
		if (channel[i]!=NULL){
			flags = fcntl(channel[i]->child_to_parent_fd[0], F_GETFL);
			fcntl(channel[i]->child_to_parent_fd[0], F_SETFL, flags | O_NONBLOCK);
			if (read(channel[i]->child_to_parent_fd[0],buff,sizeof(struct child_req_to_parent))!=-1){
				 switch (buff->type){
					 case 0:
					 				open_tabs++;
									channel[open_tabs] =(comm_channel*) malloc(sizeof(comm_channel));
									pipe(channel[open_tabs]->parent_to_child_fd);
									pipe(channel[open_tabs]->child_to_parent_fd);
					 				printf("CREATE_TAB\n");
									if ((tab_pid[open_tabs]=fork()) == 0){
										url_rendering_process(open_tabs,channel[open_tabs]);
										exit(0);
									}
									break;
						case 1: printf("NEW_URI_ENTERED\n");
										new_uri_req *request_uri = (new_uri_req*) malloc(sizeof(new_uri_req));
										request_uri->render_in_tab = buff->req.new_uri_req.render_in_tab;
										request_uri->uri = buff->req.new_uri_req.uri;
										write(channel[render_in_tab]->parent_to_child_fd[1],request_uri, sizeof(new_uri_req) );
										break;
						case 2: printf("TAB_KILLED\n");
										if (i==0){
											//cleanup all tabs open and exit!
											exit(0);
										}
										break;
				 }

			 }
			 usleep(1000);
		 }
	 }
 }

	//   handle received messages:
	//     CREATE_TAB:
	//       Prepare communication pipes with a new URL-RENDERING process
	//       Fork the new URL-RENDERING process
	//     NEW_URI_ENTERED:
	//       Send message to the URL-RENDERING process in which the new url is going to be rendered
	//     TAB_KILLED:
	//       If the killed process is the CONTROLLER process, send messages to kill all URL-RENDERING processes
	//       If the killed process is a URL-RENDERING process, send message to the URL-RENDERING to kill it
	//   sleep some time if no message received
	//
	return 0;
}

int main() {
	return router_process();
}
