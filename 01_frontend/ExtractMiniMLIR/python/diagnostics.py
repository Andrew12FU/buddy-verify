"""Organize real-declaration blockers and native/compiler failures separately."""
import re


def diagnose(records, log):
    """Return an evidence-based report; a diagnostic is never a generation rule.

    Args: records is the extractor JSON, log is the candidate Clang log text.
    Each blocker receives observed use sites or type-dependency reasons where
    available. Error strings only classify failures; they never add declarations.
    """
    blocked=[]
    for entry in records['unsupported']:
        item=dict(entry)
        key=entry.get('id')
        item['observed_uses']=[{'line':u.get('use_line'),'column':u.get('use_column'),
                                'operation':u.get('operation'),'selected':u['id']}
                              for u in records['interfaces']
                              if key and key in (u.get('id'),u.get('real_owner'),u.get('generated_owner'))]
        item['dependency_reasons']=[reason for t in records['types'] if key==t['id'] for reason in t['reasons']]
        blocked.append(item)
    categories={}
    for line in log.splitlines():
        if 'error:' not in line:continue
        if re.search('no (member|type|template) named|unknown type|undeclared identifier',line):category='missing_type_or_member'
        elif re.search('convert|conversion|no viable',line):category='missing_conversion_or_overload'
        elif 'template' in line:category='template_definition'
        else:category='declaration_generation'
        categories.setdefault(category,[]).append(line)
    return {'unsupported':blocked,'candidate_diagnostics':categories,
            'boundary':'Compiler diagnostics identify gaps; they do not authorize signature changes.'}
