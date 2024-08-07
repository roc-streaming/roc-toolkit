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

# create worktree with unique name and path and chdir to it
def enter_worktree():
    def random_dir():
        while True:
            path = '/tmp/rgh_' + ''.join(random.choice(string.ascii_lowercase + string.digits)
                for _ in range(8))
            if not os.path.exists(path):
                return path

    old_path = os.path.abspath(os.getcwd())
    new_path = random_dir()

    run_cmd([
        'git', 'worktree', 'add', '--no-checkout', new_path
        ])

    print_cmd(['cd', new_path])
    if not DRY_RUN:
        os.chdir(new_path)

    return old_path

# remove worktree and chdir back to repo
def leave_worktree(old_path):
    new_path = os.path.abspath(os.getcwd())

    print_cmd(['cd', old_path])
    os.chdir(old_path)

    run_cmd([
        'git', 'worktree', 'remove', '-f', os.path.basename(new_path)
        ])

# return current head
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

# restore remembered head
def restore_ref(ref):
    if os.path.exists('.git/index.lock'):
        run_cmd(['git', 'rebase', '--abort'])

    run_cmd(['git', 'checkout', ref])

# delete remembered head
def delete_ref(ref):
    run_cmd(['git', 'branch', '-D', ref])

# detect issue number from PR text
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

# format prefix for commit message
def make_prefix(org, repo, issue_link):
    if not issue_link:
        error("can't determine issue associated with pr\n"
              "add issue number to pr description or use --issue or --no-issue")

    issue_org, issue_repo, issue_number = issue_link

    if issue_org == org and issue_repo == repo:
        return f'gh-{issue_number}'
    else:
        return f'{issue_org}/{issue_repo}#{issue_number}'

# format commit message
def make_message(org, repo, issue_link, pr_title):
    pr_title = re.sub(r'^([Ii]ssue\s+\d+(:\s*)?)', '', pr_title)
    pr_title = re.sub(r'\s*\(?#\d+\)?$', '', pr_title)
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
        'pr_author': response['user']['login'],
        'pr_state': response['state'],
        'pr_draft': response['draft'],
        'pr_mergeable': response['mergeable'],
        'pr_rebaseable': response['rebaseable'],
        # branch in pr author's repo
        'source_branch': response['head']['ref'],
        'source_sha': response['head']['sha'],
        'source_remote': response['head']['repo']['ssh_url'],
        # branch in upstream repo
        'target_branch': response['base']['ref'],
        'target_remote': response['base']['repo']['ssh_url'],
    }

    if not no_git:
        try:
            pr_info['target_sha'] = subprocess.run(
                ['git', 'ls-remote', pr_info['target_remote'], pr_info['target_branch']],
                capture_output=True, text=True, check=True).stdout.split()[0]
        except subprocess.CalledProcessError as e:
            error("can't determine target commit")

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

    try:
        subprocess.run(
            ['gh', 'api', f'/orgs/{org}/members/'+pr_info['pr_author']],
            capture_output=True, text=True, check=True)
        pr_info['pr_contrib'] = False
    except subprocess.CalledProcessError as e:
        pr_info['pr_contrib'] = True

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

# find commit in target branch from which PR's branch was forked
@functools.cache
def find_pr_fork_point(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    try:
        # find oldest commit in current branch that is not present in target
        first_commit = subprocess.run(
            ['git', 'rev-list', '--first-parent', '^'+pr_info['target_sha'], pr_info['source_sha']],
            capture_output=True, text=True, check=True).stdout.split()[-1].strip()
        # find its parent
        fork_point = subprocess.run(
            ['git', 'rev-parse', first_commit+'^'],
            capture_output=True, text=True, check=True).stdout.strip()
    except subprocess.CalledProcessError as e:
        error("can't determine fork point")

    return fork_point

# print info about PR and linked issue
def show_pr(org, repo, pr_number, show_json):
    pr_info = query_pr_info(org, repo, pr_number, no_git=True)
    pr_actions = query_pr_actions(org, repo, pr_number, no_git=True)
    pr_commits = query_pr_commits(org, repo, pr_number, no_git=True)

    json_result = OrderedDict()
    json_section = {}

    def start_section(name, ctor=OrderedDict):
        if show_json:
            nonlocal json_result, json_section
            json_section = ctor()
            json_result[name.replace(' ', '_')] = json_section
        else:
            print(f'{Fore.GREEN}{Style.BRIGHT}{name}:{Style.RESET_ALL}')

    def add_field(key, val, color=None):
        if show_json:
            nonlocal json_section
            json_section[key] = val
        else:
            if color:
                print(f'    {key}: {color}{Style.BRIGHT}{val}{Style.RESET_ALL}')
            else:
                print(f'    {key}: {val}')

    def add_line(val, color=None):
        if show_json:
            pass
        else:
            if color:
                print(f'  {color}{Style.BRIGHT}{val}{Style.RESET_ALL}')
            else:
                print(f'  {val}')

    def add_commit(sha, msg, author, email):
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

    start_section('pull request')
    add_field('title', pr_info['pr_title'], Fore.BLUE)
    add_field('url', pr_info['pr_url'])
    add_field('author', pr_info['pr_author'])
    add_field('milestone', str(pr_info['pr_milestone']).lower(),
           Fore.MAGENTA if pr_info['pr_milestone'] is not None else Fore.RED)
    add_field('source', pr_info['source_branch'], Fore.CYAN)
    add_field('target', pr_info['target_branch'], Fore.CYAN)
    add_field('state', pr_info['pr_state'],
          Fore.MAGENTA if pr_info['pr_state'] == 'open' else Fore.RED)
    add_field('draft', str(pr_info['pr_draft']).lower(),
          Fore.MAGENTA if not pr_info['pr_draft'] else Fore.RED)
    add_field('contrib', str(pr_info['pr_contrib']).lower(), Fore.CYAN)
    add_field('mergeable', str(pr_info['pr_mergeable'] \
                            if pr_info['pr_mergeable'] is not None else 'unknown').lower(),
          Fore.MAGENTA if pr_info['pr_mergeable'] == True else Fore.RED)
    add_field('rebaseable', str(pr_info['pr_rebaseable'] \
                            if pr_info['pr_rebaseable'] is not None else 'unknown').lower(),
          Fore.MAGENTA if pr_info['pr_rebaseable'] == True else Fore.RED)

    start_section('issue')
    if pr_info['issue_link']:
        add_field('title', pr_info['issue_title'], Fore.BLUE)
        add_field('url', pr_info['issue_url'])
        add_field('milestone', str(pr_info['issue_milestone']).lower(),
               Fore.MAGENTA if pr_info['issue_milestone'] is not None else Fore.RED)
    else:
        add_line('not found', Fore.RED)

    start_section('actions')
    has_actions = False
    for action_name, action_result in pr_actions:
        has_actions = True
        add_field(action_name, action_result,
              Fore.MAGENTA if action_result == 'success' else Fore.RED)
    if not has_actions:
        add_line('not found', Fore.RED)

    start_section('commits', list)
    has_commits = False
    for commit_sha, commit_msg, commit_author, commit_email in pr_commits:
        has_commits = True
        add_commit(commit_sha, commit_msg, commit_author, commit_email)
    if not has_commits:
        add_line('not found', Fore.RED)

    if show_json:
        print(json.dumps(json_result, indent=2))

# die if PR does not fulfill all requirements
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

# checkout PR's branch
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

# update PR meta-data on github
# (link issue to PR, set milestone of PR and issue, etc)
def update_pr_metadata(org, repo, pr_number, issue_number, issue_milestone,
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

# fetch source and target commits
def fetch_pr_commits(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    run_cmd([
        'git', 'fetch', pr_info['source_remote'], pr_info['source_sha']
        ])

    run_cmd([
        'git', 'fetch', pr_info['target_remote'], pr_info['target_sha']
        ])

# squash all commits in PR's local branch into one
# invoked before rebase
def squash_pr_commits(org, repo, pr_number, title, no_issue):
    pr_info = query_pr_info(org, repo, pr_number)

    # build and validate commit message
    commit_message = make_message(
        org, repo,
        pr_info['issue_link'] if not no_issue else None,
        title or pr_info['pr_title'])

    if len(commit_message) > 72:
        error("commit message too long, use --title to overwrite")

    # merge target into PR's branch
    run_cmd([
        'git', 'merge', '--no-edit', pr_info['target_sha'],
        ])

    # find where PR's branch forked from target branch
    fork_point = find_pr_fork_point(org, repo, pr_number)

    # squash all commits since fork point into one
    run_cmd([
        'git', 'reset', '--soft', fork_point,
        ])
    run_cmd([
        'git', 'commit', '-C', pr_info['source_sha'],
        ])

    # edit message
    run_cmd([
        'git', 'commit', '--amend', '--no-edit', '-m', commit_message,
        ])

# rebase PR's local branch on its target branch
def rebase_pr_commits(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    # find where PR's branch forked from target branch
    fork_point = find_pr_fork_point(org, repo, pr_number)

    # rebase commits from fork point to HEAD on target
    run_cmd([
        'git', 'rebase', '--onto', pr_info['target_sha'], fork_point
        ])

# add issue prefix to every commit in PR's local branch
# invoked after rebase
def reword_pr_commits(org, repo, pr_number, title, no_issue):
    pr_info = query_pr_info(org, repo, pr_number)

    if no_issue:
        commit_prefix = ''
    else:
        commit_prefix = make_prefix(org, repo, pr_info['issue_link']) + ': '

    target_sha = pr_info['target_sha']

    if title:
        title = title.replace(r',', r'\,')
        sed = f"sed -r"+\
            f" -e '1s,^.*$,{commit_prefix}{title},'"
    else:
        sed = f"sed -r"+\
            f" -e '1s,^(gh-[0-9]+:? +|{org}/[^ ]+ +|[Ii]ssue *[0-9]+:? +)?,{commit_prefix},'"+\
            f" -e '1s,\s*\(?#[0-9]+\)?$,,'"

    run_cmd([
        'git', 'filter-branch', '-f', '--msg-filter', sed,
        f'{target_sha}..HEAD',
        ],
        env={'FILTER_BRANCH_SQUELCH_WARNING':'1'})

# print commits from local PR's branch
# invoked after rebase
def print_pr_commits(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    target_sha = pr_info['target_sha']

    run_cmd([
        'git', 'log', '--format=%h %s (%an <%ae>)',
        f'{target_sha}..HEAD',
        ])

# force-push PR's local branch to upstream
def force_push_pr(org, repo, pr_number):
    pr_info = query_pr_info(org, repo, pr_number)

    local_branch = os.path.basename(os.getcwd())
    source_branch = pr_info['source_branch']
    source_remote = pr_info['source_remote']

    run_cmd([
        'git', 'push', '-f',
        source_remote,
        f'{local_branch}:{source_branch}'
        ])

# tell github to merge PR
def merge_pr(org, repo, pr_number):
    # wait until PR is mereable
    while True:
        query_pr_info.cache_clear()

        pr_info = query_pr_info(org, repo, pr_number)
        if pr_info['pr_mergeable'] is not None \
            and pr_info['pr_rebaseable'] is not None:
            break

        time.sleep(0.1)

    # wait more
    time.sleep(1)

    def retry_fn(output):
        return 'GraphQL: Base branch was modified' in output or \
            'GraphQL: Pull Request is not mergeable' in output

    # tell to merge, retry if needed
    run_cmd([
        'gh', 'pr', 'merge',
        '--repo', f'{org}/{repo}',
        '--rebase',
        '--delete-branch',
        pr_number,
        ],
        retry_fn=retry_fn)

# rebase current branch on base branch
# like normal rebase, but preserves original committer name, email, and date
def stb_rebase(base_branch):
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

subparsers = parser.add_subparsers(dest='command')

stb_rebase_parser = subparsers.add_parser(
    'stb_rebase', parents=[common_parser],
    help="rebase local branch preserving author and date")
stb_rebase_parser.add_argument('base_branch', action='store_true')

show_pr_parser = subparsers.add_parser(
    'show_pr', parents=[common_parser],
    help="show pull request info")
show_pr_parser.add_argument('pr_number', type=int)
show_pr_parser.add_argument('--json', action='store_true', dest='json',
                            help="output in json format")

merge_pr_parser = subparsers.add_parser(
    'merge_pr', parents=[common_parser],
    help="squash-merge or rebase-merge pull request")
merge_pr_parser.add_argument('pr_number', type=int)
merge_pr_parser.add_argument('--rebase', action='store_true',
                             help='merge using rebase')
merge_pr_parser.add_argument('--squash', action='store_true',
                             help='merge using squash')
merge_pr_parser.add_argument('-t', '--title', dest='title',
                             help='overwrite commit message title')
merge_pr_parser.add_argument('--issue', type=int, dest='issue_number',
                             help="overwrite issue to link with")
merge_pr_parser.add_argument('--no-issue', action='store_true', dest='no_issue',
                             help="don't link issue")
merge_pr_parser.add_argument('-m', '--milestone', type=str, dest='milestone_name',
                             help="overwrite issue milestone")
merge_pr_parser.add_argument('-M', '--no-milestone', action='store_true', dest='no_milestone',
                             help="don't set issue milestone")
merge_pr_parser.add_argument('--no-checks', action='store_true', dest='no_checks',
                             help="proceed even if pr checks are failed")
merge_pr_parser.add_argument('--no-push', action='store_true', dest='no_push',
                             help="don't actually push and merge anything")
merge_pr_parser.add_argument('-n', '--dry-run', action='store_true', dest='dry_run',
                             help="don't actually run commands, just print them")

args = parser.parse_args()

if hasattr(args, 'dry_run'):
    DRY_RUN = args.dry_run

colorama.init()

if args.command == 'stb_rebase':
    stb_rebase(args.base_branch)
    exit(0)

if args.command == 'show_pr':
    show_pr(args.org, args.repo, args.pr_number, args.json)
    exit(0)

if args.command == 'merge_pr':
    if int(bool(args.rebase)) + int(bool(args.squash)) != 1:
        error("either --rebase or --squash should be specified")
    verify_pr(args.org, args.repo, args.pr_number, args.issue_number,
              args.milestone_name, args.no_checks, args.no_issue, args.no_milestone)
    # create new worktree in /tmp, where we'll checkout pr's branch
    orig_path = enter_worktree()
    merged = False
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        pr_ref = remember_ref()
        # first update metadata, so that subsequent calls to query_xxx_info()
        # will return correct values
        update_pr_metadata(args.org, args.repo, args.pr_number, args.issue_number,
                           args.milestone_name, args.no_issue, args.no_milestone)
        # ensure that all commits we're going to manipulate are available locally
        fetch_pr_commits(args.org, args.repo, args.pr_number)
        if args.squash:
            # if we're going to squash-merge, then squash commits before rebasing
            # this squash-merge will work even when rebase-merge produces conflicts
            squash_pr_commits(args.org, args.repo, args.pr_number, args.title, args.no_issue)
        # no matter if we do squash-merge or rebase-merge, rebase pr on target
        rebase_pr_commits(args.org, args.repo, args.pr_number)
        if args.rebase:
            # if we're doing rebase-merge, we must preserve original commits,
            # but ensure that each commit message has correct prefix
            reword_pr_commits(args.org, args.repo, args.pr_number, args.title, args.no_issue)
        print_pr_commits(args.org, args.repo, args.pr_number)
        if not args.no_push:
            force_push_pr(args.org, args.repo, args.pr_number)
            merge_pr(args.org, args.repo, args.pr_number)
            merged = True
    finally:
        # remove worktree in /tmp
        leave_worktree(orig_path)
        if merged:
            # delete temp branch (but only on success)
            delete_ref(pr_ref)
    exit(0)
