#! /usr/bin/env python3

from collections import OrderedDict
from colorama import Fore, Style
import argparse
import colorama
import functools
import json
import os
import os.path
import random
import re
import string
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

def run_cmd(cmd, input=None, env=None, retry_fn=None):
    cmd = [str(c) for c in cmd]

    if input:
        input = input.encode()
    if env:
        e = os.environ.copy()
        e.update(env)
        env = e
    stdout = None
    if retry_fn:
        stdout = subprocess.PIPE

    max_tries = 6

    while True:
        try:
            print_cmd(cmd)
            if DRY_RUN:
                return
            proc = subprocess.run(cmd, input=input, stdout=stdout, env=env, check=True)
            if stdout is not None:
                output = proc.stdout.decode()
                print(output, end='')
        except subprocess.CalledProcessError as e:
            output = ''
            if e.output:
                output = e.output.decode()
            if retry_fn is not None and retry_fn(output) and max_tries > 0:
                print('Retrying...')
                max_tries -= 1
                time.sleep(1)
                continue
            error('command failed')
        return

def random_worktree():
    while True:
        path = '/tmp/rgh_' + ''.join(random.choice(string.ascii_lowercase + string.digits)
            for _ in range(8))
        if not os.path.exists(path):
            return path

def enter_worktree():
    old_path = os.path.abspath(os.getcwd())
    new_path = random_worktree()

    run_cmd([
        'git', 'worktree', 'add', '--no-checkout', new_path
        ])

    print_cmd(['cd', new_path])
    if not DRY_RUN:
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

def guess_issue(org, repo, text):
    if not text:
        return None

    delim = r'[,:;?!()\[\]|+*_~<> \t\n\r]'

    prefix = f'(?:^|(?<={delim}))'
    suffix = f'(?:$|(?={delim}))'

    patterns = [
        r'(?:#|gh-)(\d+)',
        r'([\w-]+)/([\w-]+)#(\d+)',
        r'https?://github.com/([\w-]+)/([\w-]+)/issues/(\d+)(?:#[\w\d-]+)?',
    ]

    regexp = re.compile('|'.join([prefix + p + suffix for p in patterns]),
        re.IGNORECASE | re.M)

    for m in regexp.finditer(text):
        if m.group(1):
            return org, repo, int(m.group(1))

        if m.group(2) == org:
            return m.group(2), m.group(3), int(m.group(4))

        if m.group(5) == org:
            return m.group(5), m.group(6), int(m.group(7))

    return None

def make_prefix(org, repo, issue_link):
    if not issue_link:
        error("can't determine issue associated with pr\n"
              "add issue number to pr description or use --issue or --no-issue")

    issue_org, issue_repo, issue_number = issue_link

    if issue_org == org and issue_repo == repo:
        return f'gh-{issue_number}'
    else:
        return f'{issue_org}/{issue_repo}#{issue_number}'

def make_message(org, repo, issue_link, pr_title):
    pr_title = re.sub(r'\s*\(#\d+\)$', '', pr_title)
    pr_title = re.sub(r'\.$', '', pr_title)

    if issue_link:
        return '{}: {}'.format(
            make_prefix(org, repo, issue_link),
            pr_title)
    else:
        return pr_title

@functools.cache
def query_issue_info(org, repo, issue_number):
    issue_info = {}

    try:
        response = json.loads(subprocess.run([
            'gh', 'api',
            f'/repos/{org}/{repo}/issues/{issue_number}'
            ],
            capture_output=True, text=True, check=True).stdout)
    except subprocess.CalledProcessError as e:
        error(f'failed to retrieve issue info: {e.stderr.strip()}')

    issue_info['issue_title'] = response['title']
    issue_info['issue_url'] = response['html_url']

    if response['milestone']:
        issue_info['issue_milestone'] = response['milestone']['title']
    else:
        issue_info['issue_milestone'] = None

    return issue_info

@functools.cache
def query_pr_info(org, repo, pr_number, no_git=False):
    try:
        response = json.loads(subprocess.run(
            ['gh', 'api', f'/repos/{org}/{repo}/pulls/{pr_number}'],
            capture_output=True, text=True, check=True).stdout)
    except subprocess.CalledProcessError as e:
        error(f'failed to retrieve pr info: {e.stderr.strip()}')

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
        'target_remote': response['base']['repo']['ssh_url'],
    }

    if response['milestone']:
        pr_info['pr_milestone'] = response['milestone']['title']
    else:
        pr_info['pr_milestone'] = None

    pr_info['issue_link'] = None

    if 'body' in response:
        pr_info['issue_link'] = pr_info['issue_link_in_body'] = \
          guess_issue(org, repo, response['body'])

    if not pr_info['issue_link'] and 'title' in response:
        pr_info['issue_link'] = guess_issue(org, repo, response['title'])

    if not pr_info['issue_link'] and 'issue' in response:
        pr_info['issue_link'] = (org, repo, int(response['issue']['number']))

    if pr_info['issue_link']:
        issue_info = query_issue_info(*pr_info['issue_link'])
        pr_info.update(issue_info)

    if not no_git:
        try:
            pr_info['base_sha'], pr_info['base_ref'] = subprocess.run(
                ['git', 'ls-remote', pr_info['target_remote'], pr_info['target_branch']],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=True).stdout.decode().strip().split()
        except subprocess.CalledProcessError as e:
            error(f'failed to retrieve git remote info: {e.stderr.decode().strip()}')

    return pr_info

@functools.cache
def query_pr_actions(org, repo, pr_number, no_git=False):
    pr_info = query_pr_info(org, repo, pr_number, no_git)

    try:
        response = json.loads(subprocess.run([
            'gh', 'api',
            f'/repos/{org}/{repo}/actions/runs?event=pull_request',
            ],
            capture_output=True, text=True, check=True).stdout)
    except subprocess.CalledProcessError as e:
        error(f'failed to retrieve workflow runs: {e.stderr.strip()}')

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
def query_pr_commits(org, repo, pr_number, no_git=False):
    try:
        response = json.loads(subprocess.run([
            'gh', 'pr', 'view',
            '--repo', f'{org}/{repo}',
            '--json', 'commits',
            str(pr_number),
            ],
            capture_output=True, text=True, check=True).stdout)
    except subprocess.CalledProcessError as e:
        error(f'failed to retrieve pr commits: {e.stderr.strip()}')

    results = []
    for commit in response['commits']:
        results.append((commit['oid'],
                commit['messageHeadline'],
                commit['authors'][0]['name'], commit['authors'][0]['email']))

    return results

def show_pr(org, repo, pr_number, show_json):
    pr_info = query_pr_info(org, repo, pr_number, no_git=True)
    pr_actions = query_pr_actions(org, repo, pr_number, no_git=True)
    pr_commits = query_pr_commits(org, repo, pr_number, no_git=True)

    json_result = OrderedDict()
    json_section = {}

    def section(name, ctor=OrderedDict):
        if show_json:
            nonlocal json_result, json_section
            json_section = ctor()
            json_result[name.replace(' ', '_')] = json_section
        else:
            print(f'{Fore.GREEN}{Style.BRIGHT}{name}:{Style.RESET_ALL}')

    def keyval(key, val, color=None):
        if show_json:
            nonlocal json_section
            json_section[key] = val
        else:
            if color:
                print(f'    {key}: {color}{Style.BRIGHT}{val}{Style.RESET_ALL}')
            else:
                print(f'    {key}: {val}')

    def empty(val, color=None):
        if show_json:
            pass
        else:
            if color:
                print(f'  {color}{Style.BRIGHT}{val}{Style.RESET_ALL}')
            else:
                print(f'  {val}')

    def commit(sha, msg, author, email):
        if show_json:
            nonlocal json_section
            json_section.append({
                'sha': sha,
                'message': msg,
                'author': author,
                'email': email,
            })
        else:
            if 'users.noreply.github.com' in email:
                email = 'noreply.github.com'
            print(f'  {sha[:8]} {Fore.BLUE}{Style.BRIGHT}{msg}{Style.RESET_ALL}'+
                f' ({author} <{email}>)')

    def end():
        if show_json:
            print(json.dumps(json_result, indent=2))

    section('pull request')
    keyval('title', pr_info['pr_title'], Fore.BLUE)
    keyval('url', pr_info['pr_url'])
    keyval('milestone', str(pr_info['pr_milestone']).lower(),
           Fore.MAGENTA if pr_info['pr_milestone'] is not None else Fore.RED)
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
        keyval('milestone', str(pr_info['issue_milestone']).lower(),
               Fore.MAGENTA if pr_info['issue_milestone'] is not None else Fore.RED)
    else:
        empty('not found', Fore.RED)

    section('actions')
    has_actions = False
    for action_name, action_result in pr_actions:
        has_actions = True
        keyval(action_name, action_result,
              Fore.MAGENTA if action_result == 'success' else Fore.RED)
    if not has_actions:
        empty('not found', Fore.RED)

    section('commits', list)
    has_commits = False
    for commit_sha, commit_msg, commit_author, commit_email in pr_commits:
        has_commits = True
        commit(commit_sha, commit_msg, commit_author, commit_email)
    if not has_commits:
        empty('not found', Fore.RED)

    end()

def verify_pr(org, repo, pr_number, issue_number, issue_miletsone, no_checks,
              no_issue, no_milestone):
    pr_info = query_pr_info(org, repo, pr_number)

    if not no_issue:
        if issue_number:
            issue_info = query_issue_info(org, repo, issue_number)
        else:
            issue_info = pr_info

        if not issue_number and not pr_info['issue_link']:
            error("can't determine issue associated with pr\n"
                  "add issue number to pr description or use --issue or --no-issue")

        if not no_milestone:
            if not issue_miletsone and not issue_info['issue_milestone']:
                error("can't determine milestone associated with issue\n"
                      "assign milestone to issue or use --milestone or --no-milestone")

    if pr_info['pr_state'] != 'open':
        error("can't proceed on non-open pr")

    if pr_info['pr_draft']:
        error("can't proceed on draft pr")

    if not no_checks:
        for action_name, action_result in query_pr_actions(org, repo, pr_number):
            if action_result != 'success':
                error("can't proceed on pr with failed checks\n"
                      "use --no-checks to proceed anyway")

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

def update_pr(org, repo, pr_number, issue_number, issue_milestone,
              no_issue, no_milestone):
    def update_link_to_issue():
        pr_info = query_pr_info(org, repo, pr_number)

        nonlocal issue_number
        if not issue_number:
            if pr_info['issue_link']:
                _, _, issue_number = pr_info['issue_link']

        is_uptodate = pr_info['issue_link'] and \
            (pr_info['issue_link'] == pr_info['issue_link_in_body']) and \
            (pr_info['issue_link'] == (org, repo, issue_number))

        if is_uptodate:
            return

        try:
            response = json.loads(subprocess.run(
                ['gh', 'api', f'/repos/{org}/{repo}/pulls/{pr_number}'],
                capture_output=True, text=True, check=True).stdout)
        except subprocess.CalledProcessError as e:
            error(f'failed to retrieve pr info: {e.stderr.strip()}')

        body = '{}\n\n{}'.format(
            make_prefix(org, repo, (org, repo, issue_number)),
            (response['body'] or '').strip()).strip()

        run_cmd([
            'gh', 'pr', 'edit',
            '--repo', f'{org}/{repo}',
            '--body-file', '-',
            pr_number,
            ],
            input=body)

        query_pr_info.cache_clear()

    def update_milestone_of_issue():
        pr_info = query_pr_info(org, repo, pr_number)

        is_uptodate = pr_info['issue_milestone'] and \
            (not issue_milestone or pr_info['issue_milestone'] == issue_milestone)

        if is_uptodate:
            return

        issue_org, issue_repo, issue_number = pr_info['issue_link']

        run_cmd([
            'gh', 'issue', 'edit',
            '--repo', f'{issue_org}/{issue_repo}',
            '--milestone', issue_milestone,
            issue_number,
            ])

        query_issue_info.cache_clear()
        query_pr_info.cache_clear()

    def update_milestone_of_pr():
        pr_info = query_pr_info(org, repo, pr_number)

        is_uptodate = pr_info['pr_milestone'] and \
            pr_info['pr_milestone'] == pr_info['issue_milestone']

        if is_uptodate:
            return

        run_cmd([
            'gh', 'pr', 'edit',
            '--repo', f'{org}/{repo}',
            '--milestone', pr_info['issue_milestone'],
            pr_number,
            ])

        query_pr_info.cache_clear()

    if not no_issue:
        update_link_to_issue()

        if not no_milestone:
            update_milestone_of_issue()
            update_milestone_of_pr()

def fetch_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    run_cmd([
        'git', 'fetch', pr_info['target_remote'], pr_info['base_sha'],
        ])

def rebase_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    run_cmd([
        'git', 'rebase', '--onto', pr_info['base_sha'], pr_info['target_sha'],
        ])

def link_pr(org, repo, pr_number, action, no_issue):
    pr_info = query_pr_info(org, repo, pr_number)

    if no_issue:
        commit_prefix = ''
    elif action == 'link_pr':
        commit_prefix = make_prefix(org, repo, pr_info['issue_link']) + ': '
    elif action == 'unlink_pr':
        commit_prefix = ''

    base_sha = pr_info['base_sha']

    run_cmd([
        'git', 'filter-branch', '-f', '--msg-filter',
        f"sed -r -e '1s,^(gh-[0-9]+:? +|{org}/[^ ]+ +|[Ii]ssue *[0-9]+:? +)?,{commit_prefix},'"+
          " -e '1s,\s*\(#[0-9]+\)$,,'",
        f'{base_sha}..HEAD',
        ],
        env={'FILTER_BRANCH_SQUELCH_WARNING':'1'})

def squash_pr(org, repo, pr_number, title, no_issue):
    pr_info = query_pr_info(org, repo, pr_number)

    commit_message = make_message(
        org, repo,
        pr_info['issue_link'] if not no_issue else None,
        title or pr_info['pr_title'])

    run_cmd([
        'git', 'rebase', '-i', pr_info['base_sha'],
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

    base_sha = pr_info['base_sha']

    run_cmd([
        'git', 'log', '--format=%h %s (%an <%ae>)',
        f'{base_sha}..HEAD',
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
    def retry_fn(output):
        return 'GraphQL: Base branch was modified' in output or \
            'GraphQL: Pull Request is not mergeable' in output

    time.sleep(1)

    run_cmd([
        'gh', 'pr', 'merge',
        '--repo', f'{org}/{repo}',
        '--rebase',
        '--delete-branch',
        pr_number,
        ],
        retry_fn=retry_fn)

def stealth_rebase(base_branch):
    # like normal rebase, but preserves original committer name, email, and date
    # used to do periodic rebase of "develop" branch on "master"
    cmd='%s%nexec GIT_COMMITTER_DATE="%cD" GIT_COMMITTER_NAME="%cn" GIT_COMMITTER_EMAIL="%ce"'

    run_cmd([
        'git',
        '-c' f'rebase.instructionFormat="{cmd} git commit --amend --no-edit"',
        'rebase', '-i',
        base_branch,
    ],
    env={
        'GIT_EDITOR': ':',
        'GIT_SEQUENCE_EDITOR': ':',
    })

parser = argparse.ArgumentParser(prog='rgh.py')

common_parser = argparse.ArgumentParser(add_help=False)
common_parser.add_argument('--org', default='roc-streaming',
                           help='github org')
common_parser.add_argument('--repo', default='roc-toolkit',
                           help='github repo')

pr_action_parser = argparse.ArgumentParser(add_help=False)
pr_action_parser.add_argument('--issue', type=int, dest='issue_number',
                    help="overwrite issue to link with")
pr_action_parser.add_argument('--no-issue', action='store_true', dest='no_issue',
                    help="don't link issue")
pr_action_parser.add_argument('-m', '--milestone', type=str, dest='milestone_name',
                    help="overwrite issue milestone")
pr_action_parser.add_argument('-M', '--no-milestone', action='store_true', dest='no_milestone',
                    help="don't set issue milestone")
pr_action_parser.add_argument('--no-checks', action='store_true', dest='no_checks',
                    help="proceed even if pr checks are failed")
pr_action_parser.add_argument('--no-push', action='store_true', dest='no_push',
                    help="don't actually push anything")
pr_action_parser.add_argument('-n', '--dry-run', action='store_true', dest='dry_run',
                    help="don't actually run commands, just print them")

subparsers = parser.add_subparsers(dest='command')

show_pr_parser = subparsers.add_parser(
    'show_pr', parents=[common_parser],
    help="show pull request info")
show_pr_parser.add_argument('pr_number', type=int)
show_pr_parser.add_argument('--json', action='store_true', dest='json',
                         help="output in json format")

rebase_pr_parser = subparsers.add_parser(
    'rebase_pr', parents=[common_parser, pr_action_parser],
    help="rebase pull request on base branch (keeps it open)")
rebase_pr_parser.add_argument('pr_number', type=int)

link_pr_parser = subparsers.add_parser(
    'link_pr', parents=[common_parser, pr_action_parser],
    help="link pull request description and commits to issue")
link_pr_parser.add_argument('pr_number', type=int)

unlink_pr_parser = subparsers.add_parser(
    'unlink_pr', parents=[common_parser, pr_action_parser],
    help="unlink pull request commits from issue")
unlink_pr_parser.add_argument('pr_number', type=int)

merge_pr_parser = subparsers.add_parser(
    'merge_pr', parents=[common_parser, pr_action_parser],
    help="link and merge pull request")
merge_pr_parser.add_argument('--rebase', action='store_true',
                          help='merge using rebase')
merge_pr_parser.add_argument('--squash', action='store_true',
                          help='merge using squash')
merge_pr_parser.add_argument('-t', '--title', dest='title',
                          help='overwrite commit message title')
merge_pr_parser.add_argument('pr_number', type=int)

stealth_rebase_parser = subparsers.add_parser(
    'stealth_rebase', parents=[common_parser],
    help="rebase local branch preserving author and date")
stealth_rebase_parser.add_argument('base_branch', action='store_true')

args = parser.parse_args()

if hasattr(args, 'dry_run'):
    DRY_RUN = args.dry_run

colorama.init()

if args.command == 'show_pr':
    show_pr(args.org, args.repo, args.pr_number, args.json)
    exit(0)

if args.command == 'rebase_pr':
    orig_path = enter_worktree()
    pushed = False
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        pr_ref = remember_ref()
        fetch_pr(args.org, args.repo, args.pr_number)
        rebase_pr(args.org, args.repo, args.pr_number)
        log_pr(args.org, args.repo, args.pr_number)
        if not args.no_push:
            push_pr(args.org, args.repo, args.pr_number)
            pushed = True
    finally:
        leave_worktree(orig_path)
        if pushed:
            delete_ref(pr_ref)
    exit(0)

if args.command == 'link_pr' or args.command == 'unlink_pr':
    verify_pr(args.org, args.repo, args.pr_number, args.issue_number,
              args.milestone_name, args.no_checks, args.no_issue, args.no_milestone)
    orig_path = enter_worktree()
    pushed = False
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        pr_ref = remember_ref()
        update_pr(args.org, args.repo, args.pr_number, args.issue_number,
                  args.milestone_name, args.no_issue, args.no_milestone)
        fetch_pr(args.org, args.repo, args.pr_number)
        rebase_pr(args.org, args.repo, args.pr_number)
        link_pr(args.org, args.repo, args.pr_number, args.command, args.no_issue)
        log_pr(args.org, args.repo, args.pr_number)
        if not args.no_push:
            push_pr(args.org, args.repo, args.pr_number)
            pushed = True
    finally:
        leave_worktree(orig_path)
        if pushed:
            delete_ref(pr_ref)
    exit(0)

if args.command == 'merge_pr':
    if int(bool(args.rebase)) + int(bool(args.squash)) != 1:
        error("either --rebase or --squash should be specified")
    verify_pr(args.org, args.repo, args.pr_number, args.issue_number,
              args.milestone_name, args.no_checks, args.no_issue, args.no_milestone)
    orig_path = enter_worktree()
    merged = False
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        pr_ref = remember_ref()
        update_pr(args.org, args.repo, args.pr_number, args.issue_number,
                  args.milestone_name, args.no_issue, args.no_milestone)
        fetch_pr(args.org, args.repo, args.pr_number)
        rebase_pr(args.org, args.repo, args.pr_number)
        if args.rebase:
            link_pr(args.org, args.repo, args.pr_number, 'link_pr', args.no_issue)
        else:
            squash_pr(args.org, args.repo, args.pr_number, args.title, args.no_issue)
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

if args.command == 'stealth_rebase':
    stealth_rebase(args.base_branch)
    exit(0)
