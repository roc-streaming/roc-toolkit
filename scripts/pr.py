#! /usr/bin/env python3
from colorama import Fore, Style
import argparse
import colorama
import functools
import json
import os
import os.path
import re
import subprocess
import sys
import tempfile
import time

DRY_RUN = False

def error(message):
    print(f'{Fore.RED}{Style.BRIGHT}error:{Style.RESET_ALL} {message}', file=sys.stderr)
    sys.exit(1)

def print_cmd(cmd):
    pretty = ' '.join(['"'+c+'"' if ' ' in c else c for c in map(str, cmd)])
    print(f'{Fore.YELLOW}{pretty}{Style.RESET_ALL}')

def run_cmd(cmd, input=None, env=None):
    cmd = [str(c) for c in cmd]
    print_cmd(cmd)
    if input:
        input = input.encode()
    if env:
        e = os.environ.copy()
        e.update(env)
        env = e
    try:
        if not DRY_RUN:
            subprocess.run(cmd, input=input, env=env, check=True)
    except subprocess.CalledProcessError as e:
        error('command failed')

def enter_worktree():
    old_path = os.path.abspath(os.getcwd())
    new_path = tempfile.mkdtemp()

    run_cmd([
        'git', 'worktree', 'add', '--no-checkout', new_path
        ])

    print_cmd(['cd', new_path])
    os.chdir(new_path)

    return old_path

def leave_worktree(old_path):
    new_path = os.path.abspath(os.getcwd())

    print_cmd(['cd', old_path])
    os.chdir(old_path)

    run_cmd([
        'git', 'worktree', 'remove', '-f', os.path.basename(new_path)
        ])

def remember_ref():
    if DRY_RUN:
        return 'none'
    try:
        output = subprocess.run(
            ['git', 'symbolic-ref', '--short', 'HEAD'],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=True).stdout
    except:
        output = subprocess.run(
            ['git', 'rev-parse', 'HEAD'],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=True).stdout
    return output.decode().strip()

def restore_ref(ref):
    if os.path.exists('.git/index.lock'):
        run_cmd(['git', 'rebase', '--abort'])

    run_cmd(['git', 'checkout', ref])

def delete_ref(ref):
    run_cmd(['git', 'branch', '-D', ref])

def guess_issue(org, repo, pr_body):
    if not pr_body:
        return None

    keywords = 'close closes closed fix fixes resolve resolves resolved'.split()
    keywords.append('')

    for keyword in keywords:
        suffix = r'(?:$|\s)'
        prefix = r'(?:^|\s)'
        if keyword:
            prefix += keyword + r'\s+'

        m = re.search(f'{prefix}(#|gh-)(\d+){suffix}', pr_body, re.IGNORECASE)
        if m:
            return org, repo, int(m.group(2))

        for m in re.finditer(f'{prefix}([\w-]+)/([\w-]+)#(\d+){suffix}',
            pr_body, re.IGNORECASE):
            if m.group(1) == org:
                return org, m.group(2), int(m.group(3))

    for m in re.finditer(
        r'(?:^|\s)https?://github.com/([\w-]+)/([\w-]+)/issues/(\d+)(?:$|\s)',
        pr_body, re.IGNORECASE):
        if m.group(1) == org:
            return org, m.group(2), int(m.group(3))

    return None

def make_prefix(org, repo, issue_link):
    if not issue_link:
        error("can't determine issue associated with pr")

    issue_org, issue_repo, issue_number = issue_link

    if issue_org == org and issue_repo == repo:
        return f'gh-{issue_number}'
    else:
        return f'{issue_org}/{issue_repo}#{issue_number}'

def make_message(org, repo, issue_link, pr_title):
    pr_title = re.sub(r'\s*\(#\d+\)$', '', pr_title)

    return '{} {}'.format(
        make_prefix(org, repo, issue_link),
        pr_title)

@functools.cache
def query_pr_info(org, repo, pr_number):
    try:
        response = json.loads(subprocess.run(
            ['gh', 'api', f'/repos/{org}/{repo}/pulls/{pr_number}'],
            capture_output=True, text=True, check=True).stdout)
    except subprocess.CalledProcessError as e:
        error('failed to retrieve pr info')

    pr_info = {
        'pr_link': (org, repo, pr_number),
        'pr_title': response['title'],
        'pr_url': response['html_url'],
        'pr_state': response['state'],
        'pr_draft': response['draft'],
        'pr_mergeable': response['mergeable'],
        'pr_rebaseable': response['rebaseable'],
        'source_branch': response['head']['ref'],
        'source_sha': response['head']['sha'],
        'source_remote': response['head']['repo']['ssh_url'],
        'target_branch': response['base']['ref'],
        'target_sha': response['base']['sha'],
    }

    if 'issue_link' in response:
        pr_info['issue_link'] = (org, repo, int(response['issue']['number']))
    else:
        pr_info['issue_link'] = guess_issue(org, repo, response['body'])

    if pr_info['issue_link']:
        issue_org, issue_repo, issue_number = pr_info['issue_link']

        try:
            response = json.loads(subprocess.run([
                'gh', 'api',
                f'/repos/{issue_org}/{issue_repo}/issues/{issue_number}'
                ],
                capture_output=True, text=True, check=True).stdout)
        except subprocess.CalledProcessError as e:
            error('failed to retrieve issue info')

        pr_info['issue_title'] = response['title']
        pr_info['issue_url'] = response['html_url']

    return pr_info

@functools.cache
def query_pr_actions(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    try:
        response = json.loads(subprocess.run([
            'gh', 'api',
            f'/repos/{org}/{repo}/actions/runs?event=pull_request',
            ],
            stdout=subprocess.PIPE, check=True).stdout.decode())
    except subprocess.CalledProcessError as e:
        error('failed to retrieve workflow runs')

    results = {}
    for run in response['workflow_runs']:
        if run['head_sha'] != pr_info['source_sha']:
            continue
        status = run['status']
        if status == 'completed':
            status = run['conclusion']
        results[run['name']] = status

    return sorted(results.items())

@functools.cache
def query_pr_commits(org, repo, pr_number):
    try:
        response = json.loads(subprocess.run([
            'gh', 'pr', 'view',
            '--repo', f'{org}/{repo}',
            '--json', 'commits',
            str(pr_number),
            ],
            stdout=subprocess.PIPE, check=True).stdout.decode())
    except subprocess.CalledProcessError as e:
        error('failed to retrieve pr commits')

    results = []
    for commit in response['commits']:
        results.append((commit['oid'],
                commit['messageHeadline'],
                commit['authors'][0]['name'], commit['authors'][0]['email']))

    return results

def show_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    def section(name):
        print(f'{Fore.GREEN}{Style.BRIGHT}{name}:{Style.RESET_ALL}')

    def keyval(key, val, color=None):
        if color:
            print(f'  {key}: {color}{Style.BRIGHT}{val}{Style.RESET_ALL}')
        else:
            print(f'  {key}: {val}')

    def val(val, color=None):
        if color:
            print(f'  {color}{Style.BRIGHT}{val}{Style.RESET_ALL}')
        else:
            print(f'  {val}')

    section('pull request')
    keyval('title', pr_info['pr_title'], Fore.BLUE)
    keyval('url', pr_info['pr_url'])
    keyval('source', pr_info['source_branch'], Fore.CYAN)
    keyval('target', pr_info['target_branch'], Fore.CYAN)
    keyval('state', pr_info['pr_state'],
          Fore.MAGENTA if pr_info['pr_state'] == 'open' else Fore.RED)
    keyval('draft', str(pr_info['pr_draft']).lower(),
          Fore.MAGENTA if not pr_info['pr_draft'] else Fore.RED)
    keyval('mergeable', str(pr_info['pr_mergeable'] \
                            if pr_info['pr_mergeable'] is not None else 'unknown').lower(),
          Fore.MAGENTA if pr_info['pr_mergeable'] == True else Fore.RED)
    keyval('rebaseable', str(pr_info['pr_rebaseable'] \
                            if pr_info['pr_rebaseable'] is not None else 'unknown').lower(),
          Fore.MAGENTA if pr_info['pr_rebaseable'] == True else Fore.RED)

    section('issue')
    if pr_info['issue_link']:
        keyval('title', pr_info['issue_title'], Fore.BLUE)
        keyval('url', pr_info['issue_url'])
    else:
        val('not found', Fore.RED)

    section('actions')
    has_actions = False
    for action_name, action_result in query_pr_actions(org, repo, pr_number):
        has_actions = True
        keyval(action_name, action_result,
              Fore.MAGENTA if action_result == 'success' else Fore.RED)
    if not has_actions:
        val('not found', Fore.RED)

    section('commits')
    has_commits = False
    for commit_sha, commit_msg, commit_author, commit_email in \
        query_pr_commits(org, repo, pr_number):
        has_commits = True
        if 'users.noreply.github.com' in commit_email:
            commit_email = 'noreply.github.com'
        print(f'  {commit_sha[:8]} {Fore.BLUE}{Style.BRIGHT}{commit_msg}{Style.RESET_ALL}'+
              f' ({commit_author} <{commit_email}>)')
    if not has_commits:
        val('not found', Fore.RED)

def verify_pr(org, repo, pr_number, issue_number, force):
    pr_info = query_pr_info(org, repo, pr_number)

    if not issue_number and not pr_info['issue_link']:
        error("can't determine issue associated with pr")

    if not force:
        if pr_info['pr_state'] != 'open':
            error("can't proceed on non-open pr")

        if pr_info['pr_draft']:
            error("can't proceed on draft pr")

        for action_name, action_result in query_pr_actions(org, repo, pr_number):
            if action_result != 'success':
                error("can't proceed on pr with failed checks")

def checkout_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    local_branch = os.path.basename(os.getcwd())
    target_branch = pr_info['target_branch']

    run_cmd([
        'gh', 'pr', 'checkout',
        '--repo', f'{org}/{repo}',
        '-f',
        '-b', local_branch,
        pr_number,
        ])

def update_pr(org, repo, pr_number, issue_number):
    pr_info = query_pr_info(org, repo, pr_number)

    if pr_info['issue_link'] and (
        not issue_number or pr_info['issue_link'] == (org, repo, issue_number)):
        return

    try:
        response = json.loads(subprocess.run(
            ['gh', 'api', f'/repos/{org}/{repo}/pulls/{pr_number}'],
            capture_output=True, text=True, check=True).stdout)
    except subprocess.CalledProcessError as e:
        error('failed to retrieve pr info')

    body = '{}\n\n{}'.format(
        make_prefix(org, repo, (org, repo, issue_number)),
        (response['body'] or '').lstrip())

    run_cmd([
        'gh', 'pr', 'edit',
        '--repo', f'{org}/{repo}',
        '--body-file', '-',
        pr_number,
        ],
        input=body)

    query_pr_info.cache_clear()

def link_pr(org, repo, pr_number, action):
    pr_info = query_pr_info(org, repo, pr_number)

    if action == 'link':
        commit_prefix = make_prefix(org, repo, pr_info['issue_link']) + ' '
    elif action == 'unlink':
        commit_prefix = ''

    target_sha = pr_info['target_sha']

    run_cmd([
        'git', 'filter-branch', '-f', '--msg-filter',
        f"sed -r -e '1s,^(gh-[0-9]+ +|{org}/[^ ]+ +)?,{commit_prefix},'"+
          " -e '1s,\s*\(#[0-9]+\)$,,'",
        f'{target_sha}..HEAD',
        ],
        env={'FILTER_BRANCH_SQUELCH_WARNING':'1'})

def rebase_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    target_sha = pr_info['target_sha']

    run_cmd([
        'git', 'rebase', target_sha,
        ])

def squash_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    target_sha = pr_info['target_sha']
    commit_message = make_message(org, repo, pr_info['issue_link'], pr_info['pr_title'])

    run_cmd([
        'git', 'rebase', '-i', target_sha,
        ],
        env={
            'GIT_EDITOR': ':',
            'GIT_SEQUENCE_EDITOR': "sed -i '1 ! s,^pick,fixup,g'",
        })

    run_cmd([
        'git', 'commit', '--amend', '--no-edit', '-m', commit_message,
        ])

def log_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    target_sha = pr_info['target_sha']

    run_cmd([
        'git', 'log', '--format=%h %s (%an <%ae>)',
        f'{target_sha}..HEAD',
        ])

def push_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    local_branch = os.path.basename(os.getcwd())
    source_branch = pr_info['source_branch']
    source_remote = pr_info['source_remote']

    run_cmd([
        'git', 'push', '-f',
        source_remote,
        f'{local_branch}:{source_branch}'
        ])

def wait_pr(org, repo, pr_number):
    while True:
        query_pr_info.cache_clear()

        pr_info = query_pr_info(org, repo, pr_number)
        if pr_info['pr_mergeable'] is not None \
            and pr_info['pr_rebaseable'] is not None:
            break

        time.sleep(0.1)

def merge_pr(org, repo, pr_number):
    run_cmd([
        'gh', 'pr', 'merge',
        '--repo', f'{org}/{repo}',
        '--rebase',
        '--delete-branch',
        pr_number,
        ])

parser = argparse.ArgumentParser(prog='pr.py')

common_parser = argparse.ArgumentParser(add_help=False)
common_parser.add_argument('--org', default='roc-streaming',
                           help='github org')
common_parser.add_argument('--repo', default='roc-toolkit',
                           help='github repo')
common_parser.add_argument('--issue', type=int, dest='issue_number',
                    help='overwrite issue to link with')
common_parser.add_argument('--no-push', action='store_true', dest='no_push',
                    help="don't actually push anything")
common_parser.add_argument('-n', '--dry-run', action='store_true', dest='dry_run',
                    help="don't actually run commands, just print them")
common_parser.add_argument('-f', '--force', action='store_true', dest='force',
                    help="proceed even if pr doesn't match criteria")

subparsers = parser.add_subparsers(dest='command')

show_parser = subparsers.add_parser(
    'show', parents=[common_parser],
    help='show pull request info')
show_parser.add_argument('pr_number', type=int)

link_parser = subparsers.add_parser(
    'link', parents=[common_parser],
    help='link pull request description and commits to issue')
link_parser.add_argument('pr_number', type=int)

unlink_parser = subparsers.add_parser(
    'unlink', parents=[common_parser],
    help='unlink pull request commits from issue')
unlink_parser.add_argument('pr_number', type=int)

merge_parser = subparsers.add_parser(
    'merge', parents=[common_parser],
    help='link and merge pull request')
merge_parser.add_argument('--rebase', action='store_true',
                          help='merge using rebase')
merge_parser.add_argument('--squash', action='store_true',
                          help='merge using squash')
merge_parser.add_argument('pr_number', type=int)

args = parser.parse_args()

DRY_RUN = args.dry_run

colorama.init()

if args.command == 'show':
    show_pr(args.org, args.repo, args.pr_number)
    exit(0)

if args.command == 'link' or args.command == 'unlink':
    verify_pr(args.org, args.repo, args.pr_number, args.issue_number, args.force)
    orig_path = enter_worktree()
    pushed = False
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        pr_ref = remember_ref()
        update_pr(args.org, args.repo, args.pr_number, args.issue_number)
        link_pr(args.org, args.repo, args.pr_number, args.command)
        log_pr(args.org, args.repo, args.pr_number)
        if not args.no_push:
            push_pr(args.org, args.repo, args.pr_number)
            pushed = True
    finally:
        leave_worktree(orig_path)
        if pushed:
            delete_ref(pr_ref)
    exit(0)

if args.command == 'merge':
    if int(bool(args.rebase)) + int(bool(args.squash)) != 1:
        error("either --rebase or --squash should be specified")
    verify_pr(args.org, args.repo, args.pr_number, args.issue_number, args.force)
    orig_path = enter_worktree()
    merged = False
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        pr_ref = remember_ref()
        update_pr(args.org, args.repo, args.pr_number, args.issue_number)
        if args.rebase:
            rebase_pr(args.org, args.repo, args.pr_number)
            link_pr(args.org, args.repo, args.pr_number, 'link')
        else:
            squash_pr(args.org, args.repo, args.pr_number)
        log_pr(args.org, args.repo, args.pr_number)
        if not args.no_push:
            push_pr(args.org, args.repo, args.pr_number)
            wait_pr(args.org, args.repo, args.pr_number)
            merge_pr(args.org, args.repo, args.pr_number)
            merged = True
    finally:
        leave_worktree(orig_path)
        if merged:
            delete_ref(pr_ref)
    exit(0)
