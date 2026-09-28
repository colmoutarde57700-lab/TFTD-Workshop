from pathlib import Path
import json,re,html
root=Path(__file__).resolve().parent.parent
langs=['fr','en','es','de'];cats=[json.loads((root/'locales'/f'{l}.json').read_text(encoding='utf-8')) for l in langs];sources=json.loads((root/'locales/sources.json').read_text(encoding='utf-8'));keys=list(sources)
pattern=re.compile(r'%(?:[-+ #0]*\d*(?:\.\d+)?(?:ll|l|h|z)?[diuoxXfFeEgGsc]|%)')
for i,cat in enumerate(cats):
 assert set(cat)==set(keys),f'Incomplete catalogue {langs[i]}'
 for k in keys:assert cat[k] and pattern.findall(cat[k])==pattern.findall(cats[0][k]),(langs[i],k)
def wide(s):return 'L'+json.dumps(s,ensure_ascii=True).replace(r'\u0000',r'\0')
data='/* Generated from UTF-8 JSON locale files. */\ntypedef struct {const wchar_t*id;const wchar_t*text[4];} I18nEntry;\nstatic const I18nEntry i18nEntries[]={\n'
for k in keys:data+='{'+wide(k)+',{'+','.join(wide(cat[k]) for cat in cats)+'}},\n'
(root/'src/workshop_i18n_data.h').write_text(data+'};\n',encoding='utf-8')
chap=json.loads((root/'docs/chapters.json').read_text(encoding='utf-8'));data='/* Generated tutorial, same content as offline documentation. */\ntypedef struct {const wchar_t*title[4];const wchar_t*body[4];} TutorialChapter;\nstatic const TutorialChapter tutorialChapters[]={\n'
for c in chap:
 assert len(c['title'])==4 and len(c['body'])==4
 data+='{{'+','.join(wide(t) for t in c['title'])+'},{'+','.join(wide(t.replace('\n','\r\n')) for t in c['body'])+'}},\n'
(root/'src/workshop_tutorial_data.h').write_text(data+'};\n',encoding='utf-8')
for l,lang in enumerate(langs):
 folder=root/'docs'/lang;folder.mkdir(exist_ok=True)
 guide='# TFTD Workshop 2.12.7\n\n'+ '\n\n'.join('## '+c['title'][l]+'\n\n'+c['body'][l] for c in chap)
 (folder/'GUIDE.md').write_text(guide,encoding='utf-8')
 nav=''.join(f'<a href="#c{i}">{html.escape(c["title"][l])}</a>' for i,c in enumerate(chap))
 body=''.join(f'<section id="c{i}"><h2>{html.escape(c["title"][l])}</h2><div>{html.escape(c["body"][l])}</div></section>' for i,c in enumerate(chap))
 page=f'<!doctype html><html lang="{lang}"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>TFTD Workshop — {lang}</title><style>body{{font:17px/1.6 system-ui;margin:0;color:#dfebee;background:#101b22}}header{{padding:25px;background:#203744}}nav{{padding:20px;display:flex;flex-wrap:wrap;gap:15px;position:sticky;top:0;background:#182932}}a{{color:#83cfde}}main{{max-width:1050px;margin:auto;padding:25px}}section{{scroll-margin-top:130px;border-bottom:1px solid #42616f;padding:20px 0}}section div{{white-space:pre-wrap}}h2{{color:#b7ebd2}}</style><header><h1>TFTD Workshop 2.12.7</h1></header><nav>{nav}</nav><main>{body}</main></html>'
 (folder/'index.html').write_text(page,encoding='utf-8')
print(f'{len(keys)} complete entries x 4 languages; {len(chap)} chapters x 4; placeholders valid')

