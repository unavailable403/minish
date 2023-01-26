/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_1.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ergrigor < ergrigor@student.42yerevan.am > +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/21 19:11:28 by ergrigor          #+#    #+#             */
/*   Updated: 2023/01/27 00:14:36 by ergrigor         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../Includes/main.h"

char	**get_paths(void)
{
	t_env	*ptr;

	ptr = g_lobal->env;
	while (ptr)
	{
		if (ft_strcmp(ptr->val_name, "PATH") == 0)
			return (ft_split(ptr->val_value, ':'));
		ptr = ptr->next;
	}
	return (NULL);
}

int	is_directory(char *cmd, int flag)
{
	int	i;
	struct stat _path;

	i = 0;
	stat(cmd, &_path);
	while (cmd[i])
	{
		if (cmd[i] == '/')
			break ;
		i++;
		if (cmd[i] == '\0' && !flag)
			return (1);
	}
	if (S_ISDIR(_path.st_mode) == 1)
	{
		int	fd = open(cmd, O_WRONLY, 0644);
		return (set_status(126));
	}
	else if (access(cmd, X_OK | R_OK) == 0)
		return (1);
	return (set_status(127));
}

char	*get_abs_path(char **paths, char *cmd)
{
	int		i;
	char	*temp;

	i = 0;
	if (access(cmd, X_OK | R_OK) == 0)
		return (cmd);
	while (paths[i])
	{
		temp = ft_strjoin(paths[i], "/");
		temp = ft_strjoin(temp, cmd);
		if (access(temp, X_OK | R_OK) == 0)
			break ;
		free (temp);
		i++;
	}
	if (access(temp, X_OK | R_OK) == 0)
		return (temp);
	else
	{
		// dup2(S)
		printf("KARGIN-Shell: %s: command not found\n", cmd);
	}
	return (NULL);
}

int	is_builtin(char *cmd)
{
	if (ft_strcmp(cmd, "echo") == 0)
		return (0);
	else if (ft_strcmp(cmd, "cd") == 0)
		return (0);
	else if (ft_strcmp(cmd, "pwd") == 0)
		return (0);
	else if (ft_strcmp(cmd, "export") == 0)
		return (0);
	else if (ft_strcmp(cmd, "unset") == 0)
		return (0);
	else if (ft_strcmp(cmd, "env") == 0)
		return (0);
	else if (ft_strcmp(cmd, "exit") == 0)
		return (0);
	else
		return (1);
}

void	_execute(t_element *ptr)
{
	char	*path;
	pid_t	pid;
	int		status;
	if (!ptr->command || !ptr->command->args || ! ptr->command->args[0])
		return ;
	if (is_builtin(ptr->command->cmd) == 0)
		printf ("run_builtin(ptr->command)\n");
	else
	{
		if (is_directory(ptr->command->cmd, 0) == 1){
			pid = fork();
			if (pid == 0)
			{
				path = get_abs_path(get_paths(), ptr->command->cmd);
				if (execve(path, ptr->command->args, g_lobal->real_env) == -1)
				{
					exit(set_status(127));
				}
				exit(0);
			}
			else
				hd_wait(&status, &pid);
		}
		else if (set_status(is_directory(ptr->command->cmd, 0)) > 125)
		{
			ft_putstr_fd("KARGIN-SHELL	-$: ", 2);
			ft_putstr_fd(ptr->command->cmd, 2);
			ft_putstr_fd(": ", 2);
			ft_putstr_fd(strerror(errno), 2);
			ft_putstr_fd("\n", 2);
		}
	}
}

void	single_execution(t_element *ptr)
{
	if (!ptr->command && !ptr->command->args)
		return ;
	dup2(ptr->command->in, STDIN_FILENO);
	dup2(ptr->command->out, STDOUT_FILENO);
	_execute(ptr);
	dup2(g_lobal->all_fd[0], STDIN_FILENO);
	dup2(g_lobal->all_fd[1], STDOUT_FILENO);
	close(g_lobal->all_fd[0]);
	close(g_lobal->all_fd[1]);
	close(g_lobal->all_fd[2]);
	makefd();
}

void	execution(void)
{
	t_element	*ptr;

	ptr = g_lobal->elem;
	if (!ptr->next)
		single_execution(ptr);
	else
		pipe_execution(ptr);
}

void pipe_execution(t_element *ptr)
{
	int	(*pipes)[2];
	int	pip_count;
	int	i;
	int	counter;
	int	status;
	
	pip_count = pipe_count(ptr) - 1;
	pipes = malloc(sizeof(*pipes) * pip_count);
	if(!pipes)
		return ;
	i = -1;
	while (++i < pip_count)
	{
		if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            exit(1);
        }
	}
	i = 0;
	counter = do_pipe_execute(ptr, pipes, pip_count);
	while(i < counter)
	{
		wait(&status);
		i++;
	}
	close_all_pipes(pipes, pip_count);
	free(pipes);
}

int	do_pipe_execute(t_element *ptr, int (*pipes)[2], int pip_count)
{
	int	i;
	int	status;
	pid_t	pid;
	int		file;

	
	i = 0;
	while(ptr != NULL)
	{
		if (!ptr->command->args)
			ptr = ptr->next;
		pid = fork();
		if (pid == 0)
		{
			if (i == 0)
			{
				ptr->command->out = pipe_or_redir_out(ptr->command, pipes, i);
				dup2(ptr->command->out, STDOUT_FILENO);
			}
			else if (i > 0 && i < (pip_count - 1))
			{
				ptr->command->out = pipe_or_redir_out(ptr->command, pipes, i);
				ptr->command->in = pipe_or_redir_input(ptr->command, pipes, i);
				dup2(ptr->command->in, STDIN_FILENO);
				dup2(ptr->command->out, STDOUT_FILENO);
			}
			else
			{
				ptr->command->in = pipe_or_redir_input(ptr->command, pipes, i);
				dup2(ptr->command->in, STDIN_FILENO);
				dup2(ptr->command->out, STDOUT_FILENO);
			}
			close_all_pipes(pipes, pip_count);
			if (execve(get_abs_path(get_paths(), ptr->command->cmd), ptr->command->args, g_lobal->real_env) == -1)
			{
				close_all_pipes(pipes, pip_count);
				exit(set_status(127));
			}
			exit(0);
		}
		i++;
		ptr = ptr->next;
	}
	close_all_pipes(pipes, pip_count);
	return (i);
}

int pipe_count(t_element *ptr)
{
	int count;
	t_element	*pptr;

	count = 0;
	pptr = ptr;
	while(pptr)
	{
		count++;
		pptr = pptr->next;
	}
	return (count);
}

void	close_all_pipes(int pips[][2], int pip)
{
	int	a;

	a = -1;
	while (++a < pip)
	{
		close(pips[a][1]);
		close(pips[a][0]);
	}
}

int	pipe_or_redir_input(t_command *command, int (*pipes)[2], int i)
{
	if(command->in == g_lobal->all_fd[0] || command->in == 0)
		return (pipes[i - 1][0]);
	return (command->in);
}

int	pipe_or_redir_out(t_command *command, int (*pipes)[2], int i)
{
	if(command->out == g_lobal->all_fd[1] || command->out == 1)
		return (pipes[i][1]);
	return (command->out);
}