-- Run ONLY against a disposable local Supabase database: psql ... -v ON_ERROR_STOP=1 -f tests/supabase_rls.sql
begin;
insert into auth.users (id, email) values
 ('10000000-0000-0000-0000-000000000001','hex-test-a@example.invalid'),
 ('10000000-0000-0000-0000-000000000002','hex-test-b@example.invalid');
set local role authenticated;
select set_config('request.jwt.claim.sub','10000000-0000-0000-0000-000000000001',true);
insert into public.game_saves(owner_id,slot,state) values
 ('10000000-0000-0000-0000-000000000001','autosave',
 '{"version":1,"map_size":10,"map_seed":1,"round":1,"turn_index":0,"next_uid":1,"finished":false,"winner_id":-1,"tiles":[],"units":[],"factions":[],"fog":{}}');
do $$ begin
 if (select count(*) from public.game_saves) != 1 then raise exception 'own save not readable'; end if;
end $$;
select set_config('request.jwt.claim.sub','10000000-0000-0000-0000-000000000002',true);
do $$ begin
 if (select count(*) from public.game_saves) != 0 then raise exception 'other save leaked'; end if;
 begin
  insert into public.game_saves(owner_id,slot,state)
  select '10000000-0000-0000-0000-000000000001','foreign',
    '{"version":1,"map_size":10,"map_seed":1,"round":1,"turn_index":0,"next_uid":1,"finished":false,"winner_id":-1,"tiles":[],"units":[],"factions":[],"fog":{}}'::jsonb;
  raise exception 'foreign insert succeeded';
 exception when insufficient_privilege then null;
 end;
 update public.game_saves set slot='hacked';
 if found then raise exception 'foreign update succeeded'; end if;
 delete from public.game_saves;
 if found then raise exception 'foreign delete succeeded'; end if;
end $$;
select set_config('request.jwt.claim.sub','10000000-0000-0000-0000-000000000001',true);
do $$ begin
 if (select count(*) from public.game_saves where slot='autosave') != 1 then raise exception 'save changed by other user'; end if;
 begin
  update public.game_saves set owner_id='10000000-0000-0000-0000-000000000002';
  raise exception 'ownership transfer succeeded';
 exception when insufficient_privilege then null;
 end;
end $$;
set local role anon;
do $$ begin
 begin
  perform * from public.game_saves;
  raise exception 'anonymous read succeeded';
 exception when insufficient_privilege then null;
 end;
end $$;
rollback;
